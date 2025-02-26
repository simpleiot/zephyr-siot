#include "zpoint.h"
#include <point.h>
#include <ctype.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/shell/shell.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>

#include <lwip/opt.h>
#include <lwip/apps/snmp.h>
#include <lwip/apps/snmp_core.h>

/* mib_ats2 is declared in this file: */
#include "z-mr.mib.h"
#include "lwip/apps/snmp_mib2.h"

#define STACKSIZE 2304U
#define PRIORITY  7

LOG_MODULE_REGISTER(z_snmp, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);
ZBUS_CHAN_DECLARE(ticker_chan);

ZBUS_MSG_SUBSCRIBER_DEFINE(z_snmp_sub);
ZBUS_CHAN_ADD_OBS(point_chan, z_snmp_sub, 3);
ZBUS_CHAN_ADD_OBS(ticker_chan, z_snmp_sub, 4);

#define TRAP_INTERVAL_MSEC 2000U
#define STARTUP_DELAY_MSEC 12000U

extern const struct snmp_mib mib_ats2;
static const struct snmp_mib *mibs[] = {&mib2, &mib_ats2};

/* A leaf in the ATS2 library is identified with the name, eg. 'ats2OperationMode'.
 * The macro MK_OID()
 */

#define MK_OID(NAME)                            NAME##_oid
#define SNMP_CALL_TRAP(name, value, value_type) snmp_trap_it(#name, MK_OID(name), value, value_type)

static void snmp_trap_it(const char *name, const char *oid_string, unsigned value, unsigned type);

static int oid_string_to_array(struct snmp_obj_id *oid_result, const char *oid_string);

extern void snmp_init(void);
extern void snmp_loop(void);

static bool has_started;
static char ip_address[20] = "0.0.0.0";

/* These are the possible values of the oid called 'ats2OperationMode'
 */
typedef enum {
	opModeInvalid = 0,
	opModeInitialization = 1,
	opModeDiagnosis = 2,
	opModeOff = 3,
	opModeSource1 = 4,
	opModeSource2 = 5,
	opModeSafe = 6,
	opModeFault = 7
} ats2OperationMode;

/* The ATS mode can be any of Source1, Source2, or Off. */
static ats2OperationMode current_ats_mode = opModeSource1;

void snmp_prepare_trap_test(const char *ip_address);
static int compose_and_send_snmp_trap(const char *name, const char *oid_string, u32_t value,
				      unsigned asn_type);

static void send_ats_traps(ats2OperationMode mode);

/* This temporary function 'delay_has_passed()' will return after
 * 'duration_ms' has passed.
 */
static bool delay_has_passed(uint32_t duration_ms);

static bool has_valid_ip();

void z_snmp_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("z snmp thread");

	point p;
	const struct zbus_channel *chan;

	for (;;) {
		if (!zbus_sub_wait_msg(&z_snmp_sub, &chan, &p, K_FOREVER)) {
			if (chan == &point_chan) {
				bool doSendSource = false;
				if (strcmp(p.type, POINT_TYPE_ATS_A) == 0) {
					if (has_started) {
						current_ats_mode = opModeSource1;
						doSendSource = true;
						zephyr_log("ATS_A received.\n");
					}
				} else if (strcmp(p.type, POINT_TYPE_ATS_B) == 0) {
					if (has_started) {
						current_ats_mode = opModeSource2;
						doSendSource = true;
						zephyr_log("ATS_B received.\n");
					}
				} else if (strcmp(p.type, POINT_TYPE_SNMP_SERVER) == 0) {
					bool is_valid;
					snprintf(ip_address, sizeof ip_address, "%s", p.data);
					is_valid = has_valid_ip();
					zephyr_log("IP-address '%s' received (valid = %s).\n",
						   ip_address, is_valid ? "true" : "false");
				} else if (strcmp(p.type, "uname") == 0) {
					// echo p uname 0 STR esp32.local > /dev/ttyUSB0
					// echo p snmpServer 0 STR "192.168.2.27" > /dev/ttyUSB0
					char new_hostname[24];
					snprintf(new_hostname, sizeof new_hostname, "%s", p.data);
					size_t len = strlen(new_hostname);
					// Set the new hostname
					net_hostname_set(new_hostname, len);

					zephyr_log("uname %s\n", new_hostname);
				}

				if (doSendSource && has_valid_ip()) {
					send_ats_traps(current_ats_mode);
				}
			} else if (chan == &ticker_chan) {
				if (has_started == false) {
					/* Without a delay, the device will crash if we access
					 * the network too early. Now postponing initialisation
					 * for 12 seconds.
					 */
					if (delay_has_passed(STARTUP_DELAY_MSEC)) {
						static const struct snmp_obj_id enterprise_oid = {
							7, {1, 3, 6, 1, 4, 1, 62530, 2}};
						snmp_set_device_enterprise_oid(&enterprise_oid);
						/* Install MIB's in lwIP: 'mib2' and 'mib_ats2'. */
						snmp_set_mibs(mibs, LWIP_ARRAYSIZE(mibs));

						/* Open sockets to send traps and to receive
						 * requests. */
						snmp_init();
						/* From now on, SNMP is operational. */
						has_started = true;
					}
				} else {
					/* Check incoming packets. */
					snmp_loop();
				}
			}
		}
	}
}

extern s32_t ask_ats_mode()
{
	/* The MIB code in z-mr.mib.c may use this function
	 * as a call-back. */
	return (signed)current_ats_mode;
}

/*
 * compose_and_send_snmp_trap()
 */
#define ENTERPRISE_OID {9, {1, 3, 6, 1, 6, 3, 1, 1, 5}} // 1.3.6.1.6.3.1.1.5.1

/**
 * @brief Prepare an SNMP trap and send it. The target IP-address
 *        can be set by calling snmp_prepare_trap_test()
 *
 * @param[in] name The name of the property to send, just for logging.
 * @param[in] oid_string The OID of the property
 * @param[in] value The value to be transmitted
 * @param[in] asn_type (see defines of SNMP_ASN1_TYPE_...)
 *
 * @return An err_t, see defines of SNMP_ERR_...
 */
static int compose_and_send_snmp_trap(const char *name, const char *oid_string, u32_t value,
				      unsigned asn_type)
{
	struct snmp_varbind *varbinds = NULL;
	static const struct snmp_obj_id enterprise_oid = ENTERPRISE_OID; // Set enterprise OID
	s32_t generic_trap = 6;  // Generic trap code (e.g., 6 for enterprise specific)
	s32_t specific_trap = 1; // Specific trap code (customize as needed) ,
				 // will be appended to '1.3.6.1.6.3.1.1.5'.

	err_t err = SNMP_ERR_RESOURCEUNAVAILABLE;
	varbinds = (struct snmp_varbind *)mem_malloc(sizeof(struct snmp_varbind));

	if (varbinds != NULL) {
		memset(varbinds, 0, sizeof *varbinds);

		// Copy a string to an array of 32-bit numbers
		int rc = oid_string_to_array(&varbinds->oid, oid_string);

		varbinds->type = asn_type;           // Type of the variable: see SNMP_ASN1_TYPE_...
		varbinds->value = (void *)&value;    // Pointer to value
		varbinds->value_len = sizeof(value); // Length of value

		err = snmp_send_trap(&enterprise_oid, generic_trap, specific_trap, varbinds);

		zephyr_log("SNMP trap %s (err = %d).\n", err == ERR_OK ? "sent well" : "failed",
			   err);

		// Free allocated memory for varbinds
		mem_free(varbinds);
	} else {
		zephyr_log("Memory allocation failed for varbinds.\n");
	}
	return err;
}

static void snmp_trap_it(const char *name, const char *oid, unsigned value, unsigned type)
{
	snmp_prepare_trap_test(ip_address);
	compose_and_send_snmp_trap(name, oid, value, type);
}

static void send_ats_traps(ats2OperationMode mode)
{
	unsigned op_mode = (unsigned)mode;
	switch (op_mode) {
	case opModeSource1:
		SNMP_CALL_TRAP(ats2TrapOutputPoweredBySource1, op_mode, SNMP_ASN1_TYPE_UNSIGNED32);
		break;
	case opModeSource2:
		SNMP_CALL_TRAP(ats2TrapOutputPoweredBySource2, op_mode, SNMP_ASN1_TYPE_UNSIGNED32);
		break;
	case opModeOff:
	default:
		SNMP_CALL_TRAP(ats2TrapOutputNotPowered, op_mode, SNMP_ASN1_TYPE_UNSIGNED32);
		break;
	}
}

static bool delay_has_passed(uint32_t duration_ms)
{
	static uint32_t last_time = 0;
	bool rc = false;
	uint32_t uptime_ms = k_uptime_get();
	uint32_t diff = uptime_ms - last_time;
	if (diff >= duration_ms) {
		last_time = uptime_ms;
		rc = true;
	}
	return rc;
}

static bool has_valid_ip()
{
	bool rc = true;
	size_t index;
	size_t length = strlen(ip_address);
	for (index = 0; index < length; index++) {
		char ch = ip_address[index];
		if (isspace(ch)) {
			ip_address[index] = 0;
			break;
		}
	}
	u32_t ip = inet_addr(ip_address);
	if ((ip == 0U) || (ip == ~0U)) {
		rc = false;
	}
	return rc;
}

/**
 * @brief Copy a string like "1.3.6.1.4.1.62530" to an array of 32-bit numbers.
 *
 * @param[in] oid_string The character string representing an OID.
 * @param[out] oid_result Will contain an array of 32-bit numbers
 *
 * @return The number of digits that were stored, also stored in oid_result->len
 */
static int oid_string_to_array(struct snmp_obj_id *oid_result, const char *oid_string)
{
	const char *source = oid_string;
	int target = 0;
	memset(oid_result, 0, sizeof *oid_result);
	while (*source) {
		unsigned value = 0;
		int has_digit = 0;
		while ((*source >= '0') && (*source <= '9')) {
			value *= 10;
			value += *source - '0';
			source++;
			has_digit++;
		}
		if (!has_digit) {
			break;
		}
		oid_result->id[target++] = value;
		if (*source != '.') {
			break;
		}
		if (target >= SNMP_MAX_OBJ_ID_LEN) {
			zephyr_log("oid_string_to_array: IOD string too long (> %u)\n",
				   SNMP_MAX_OBJ_ID_LEN);
			break;
		}
		source++;
	}
	oid_result->len = target;
	/* Returns the number of digits decoded. */
	return target;
}

K_THREAD_DEFINE(z_snmp, STACKSIZE, z_snmp_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
