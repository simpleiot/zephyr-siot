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

#define STACKSIZE 2304U
#define PRIORITY  7

LOG_MODULE_REGISTER(z_snmp, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);
ZBUS_CHAN_DECLARE(ticker_chan);

ZBUS_MSG_SUBSCRIBER_DEFINE(z_snmp_sub);
ZBUS_CHAN_ADD_OBS(point_chan, z_snmp_sub, 3);
ZBUS_CHAN_ADD_OBS(ticker_chan, z_snmp_sub, 4);

#define TRAP_INTERVAL_MSEC 2000U
#define STARTUP_DELAY_MSEC 10000U

extern void snmp_init(void);
extern void snmp_loop(void);

static bool snmp_active = 1;
static bool has_started;
static char ip_address[16] = "192.168.1.23";

void snmp_prepare_trap_test(const char *ip_address);
static int compose_and_send_snmp_trap(const char *name, const char *oid_string, u32_t value,
				      unsigned asn_type);
static int oid_string_to_array(struct snmp_obj_id *oid_result, const char *oid_string);
static void send_ats_traps(void);
static int handle_sendtrap(const struct shell *shell, size_t argc, char **argv);

SHELL_CMD_REGISTER(sendtrap, NULL, "Sends a SNMP traps for testing.", handle_sendtrap);

void test_it();

static int handle_sendtrap(const struct shell *shell, size_t argc, char **argv)
{
	int index = 2;
	/* Handle shell command: "sendtrap [ <ip address> ] [ start | stop ]" */
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	for (index = 1; index < argc; index++) {
		/* Use provided an IP address. */
		if (strncmp(argv[index], "start", 5) == 0) {
			snmp_active = true;
		} else if (strncmp(argv[index], "stop", 4) == 0) {
			snmp_active = false;
		} else if (isdigit(argv[index][0])) {
			snprintf(ip_address, sizeof ip_address, "%s", argv[1]);
		}
	}
	if (argc < 2) {
		shell_print(shell, "Usage: sendtrap [ <ip address> ] [ start | stop ]");
		shell_print(shell, "IP-address now %s\n", ip_address);
	}
	test_it();

	return 0;
}

static bool time_to_trap(uint32_t duration)
{
	static uint32_t last_time = 0;
	bool rc = false;
	uint32_t uptime_ms = k_uptime_get();
	uint32_t diff = uptime_ms - last_time;
	if (diff >= duration) {
		last_time = uptime_ms;
		rc = true;
	}
	return rc;
}

void z_snmp_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("z snmp thread");

	point p;
	const struct zbus_channel *chan;

	for (;;) {
		if (!zbus_sub_wait_msg(&z_snmp_sub, &chan, &p, K_FOREVER)) {
			if (chan == &point_chan) {
				if (strcmp(p.type, POINT_TYPE_ATS_A) == 0) {
					// TODO: eventually, send ATS event out SNMP after we define
					// MIB
				} else if (strcmp(p.type, POINT_TYPE_ATS_B) == 0) {
					// TODO: eventually, send ATS event out SNMP after we define
					// MIB
				}
			} else if (chan == &ticker_chan) {
				// can do periodic stuff here
				if (snmp_active) {
					if (has_started == false) {
						if (time_to_trap(STARTUP_DELAY_MSEC)) {
							has_started = true;
							snmp_init();
						}
					} else {
						snmp_loop();
						if (time_to_trap(TRAP_INTERVAL_MSEC)) {
							/* Set the target IP-address. */
							snmp_prepare_trap_test(ip_address);
							/* Send a bunch of ATS-related traps. */
							send_ats_traps();
						}
					}
				}
			}
		}
	}
}

static void send_ats_traps()
{
	/* Here an example of how we could initiate and the ATS-related traps.
	 * I found the best documented ATS MIB's on github, by eaton.com :
	 * EATON-ATS2-MIB and EATON-OIDS
	 * Download : https://github.com/librenms/librenms/tree/master/mibs/eaton
	 * A helpful JSON can be found here:
	 *     https://mibbrowser.online/mibdb_search.php?mib=EATON-ATS2-MIB
	 */
	enum ats2OperationMode {
		opModeInvalid = 0,
		opModeInitialization = 1,
		opModeDiagnosis = 2,
		opModeOff = 3,
		opModeSource1 = 4,
		opModeSource2 = 5,
		opModeSafe = 6,
		opModeFault = 7
	};

	enum inputStatusFrequency {
		inStatFreqInvalid = 0,
		inStatFreqGood = 1,
		inStatFreqOutOfRange = 2,
	};

	enum preferredSource {
		prefSource1 = 1,
		prefSource2 = 2
	};

	unsigned voltage_in = 235 * 10; /* Unit: 0.1 Volt. */
	unsigned freq_in = 60 * 10;     /* Unit: 0.1 Hz. */
	unsigned voltage_out = 238 * 10;
	unsigned curr_out = 12 * 10; /* Unit: 0.1 A. */
	unsigned op_mode = opModeSource1;
	unsigned freq_in_stat = inStatFreqGood;
	unsigned pref_source = prefSource2;

	compose_and_send_snmp_trap("ats2InputVoltage", "1.3.6.1.4.1.534.10.2.2.2.1.2", voltage_in,
				   SNMP_ASN1_TYPE_UNSIGNED32);
	compose_and_send_snmp_trap("ats2InputFrequency", "1.3.6.1.4.1.534.10.2.2.2.1.3", freq_in,
				   SNMP_ASN1_TYPE_UNSIGNED32);
	compose_and_send_snmp_trap("ats2OutputVoltage", "1.3.6.1.4.1.534.10.2.2.3.1", voltage_out,
				   SNMP_ASN1_TYPE_UNSIGNED32);
	compose_and_send_snmp_trap("ats2OutputCurrent", "1.3.6.1.4.1.534.10.2.2.3.2", curr_out,
				   SNMP_ASN1_TYPE_UNSIGNED32);
	compose_and_send_snmp_trap("ats2OperationMode", "1.3.6.1.4.1.534.10.2.2.4", op_mode,
				   SNMP_ASN1_TYPE_UNSIGNED32);
	compose_and_send_snmp_trap("ats2InputStatusFrequency", "1.3.6.1.4.1.534.10.2.3.2.1.2",
				   freq_in_stat, SNMP_ASN1_TYPE_UNSIGNED32);
	compose_and_send_snmp_trap("ats2ConfigPreferred", "1.3.6.1.4.1.534.10.2.4.5", pref_source,
				   SNMP_ASN1_TYPE_UNSIGNED32);
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
		int index;
		memset(varbinds, 0, sizeof *varbinds);

		// Copy a string to an array of 32-bit numbers
		int rc = oid_string_to_array(&varbinds->oid, oid_string);

		varbinds->type = asn_type;           // Type of the variable: see SNMP_ASN1_TYPE_...
		varbinds->value = (void *)&value;    // Pointer to value
		varbinds->value_len = sizeof(value); // Length of value

		err = snmp_send_trap(&enterprise_oid, generic_trap, specific_trap, varbinds);

		zephyr_log("SNMP trap sent %s (err = %d).\n", err == ERR_OK ? "good" : "BAD", err);

		// Free allocated memory for varbinds
		mem_free(varbinds);
	} else {
		zephyr_log("Memory allocation failed for varbinds.\n");
	}
	return err;
}

/**
 * @brief Copy a string like "1.3.6.1.4.1.534" to an array of 32-bit numbers.
 *
 * @param[in] oid_string The character string representing an OID.
 * @param[out] oid_result Will contain an array of 32-bit numbers
 *
 * @return The number of digits that were stored, also stored in oid_result->len
 */
static int oid_string_to_array(struct snmp_obj_id *oid_result, const char *oid_string)
{
	int rc = 0;
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

static int snmp_trap_it(const char *apName, const unsigned *oid, size_t oid_size, unsigned value,
			unsigned type)
{
	zephyr_log("name  = '%s'\n", apName);
	zephyr_log("oid   = '%d.%d.%d.%d'\n", oid[0], oid[1], oid[2], oid[3]);
	zephyr_log("size  = '%u'\n", oid_size);
	zephyr_log("value = '%u'\n", value);
	zephyr_log("type  = '%u'\n", type);
}

#define MK_STRING(NAME) NAME##_str
#define MK_OID(NAME)    NAME##_oid
#define SNMP_CALL_TRAP(name, value, value_type)                                                    \
	snmp_trap_it(#name, MK_OID(name), LWIP_ARRAYSIZE(MK_OID(name)), value, value_type)

void test_it()
{
	const unsigned ats2OperationMode_oid[] = {1, 3, 6, 1, 4, 1, 534, 10, 2, 2, 4};
	unsigned op_mode = 4u;

	SNMP_CALL_TRAP(ats2OperationMode, op_mode, SNMP_ASN1_TYPE_UNSIGNED32);
}

K_THREAD_DEFINE(z_snmp, STACKSIZE, z_snmp_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
