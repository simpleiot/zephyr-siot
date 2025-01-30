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

#define STACKSIZE 2304U
#define PRIORITY  7

LOG_MODULE_REGISTER(z_snmp, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);
ZBUS_CHAN_DECLARE(ticker_chan);

ZBUS_MSG_SUBSCRIBER_DEFINE(z_snmp_sub);
ZBUS_CHAN_ADD_OBS(point_chan, z_snmp_sub, 3);
ZBUS_CHAN_ADD_OBS(ticker_chan, z_snmp_sub, 4);

#define TRAP_INTERVAL_MSEC   2000U
#define STARTUP_DELAY_MSEC  10000U

extern void snmp_init (void);
extern void snmp_loop (void);

static bool snmp_active = 1;
static bool has_started;
static char ip_address[16] = "192.168.1.23";

void snmp_prepare_trap_test(const char *ip_address);
void snmp_coldstart_trap(void);
static void compose_and_send_snmp_trap();
 
static int handle_sendtrap(const struct shell *shell, size_t argc, char **argv);

SHELL_CMD_REGISTER(sendtrap, NULL, "Sends an SNMP trap for testing.", handle_sendtrap);

static int handle_sendtrap(const struct shell *shell, size_t argc, char **argv)
{
	/* Handle shell command: "sendtrap <ip address>" */
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	if (argc >= 2) {
		/* Use provided an IP address. */
		if (strncmp (argv[1], "start", 5) == 0) {
			snmp_active = true;
		} else if (strncmp (argv[1], "stop", 4) == 0) {
			snmp_active = false;
		} else if (isdigit (argv[1][0])) {
			snprintf (ip_address, sizeof ip_address, "%s", argv[1]);
		}
	} else {
		shell_print(shell, "Please provide IP-address \"%s\", \"start\" or \"stop\".", ip_address);
	}
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
//	Calling snmp_init() immediately will crash the application.
//	This problem started when we decreased the number of network buffers
//	snmp_init();

	point p;
	const struct zbus_channel *chan;

	for (;;) {
		if (!zbus_sub_wait_msg(&z_snmp_sub, &chan, &p, K_FOREVER)) {
			if (chan == &point_chan) {
				if (strcmp(p.type, POINT_TYPE_ATS_A) == 0) {
					// TODO: eventually, send ATS event out SNMP after we define MIB
				} else if (strcmp(p.type, POINT_TYPE_ATS_B) == 0) {
					// TODO: eventually, send ATS event out SNMP after we define MIB
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
							snmp_prepare_trap_test(ip_address);
							compose_and_send_snmp_trap();
						}
					}
				}
			}
		}
	}
}

/* 1.3.6.1.6.3.1.1.5.1 : the last branch 1 will be added by lwIP. */

#define ENTERPRISE_OID {9, {1, 3, 6, 1, 6, 3, 1, 1, 5 }}

/* Function to compose and send an SNMP trap */
static void compose_and_send_snmp_trap()
{
	struct snmp_varbind *varbinds = NULL;
	struct snmp_obj_id enterprise_oid = ENTERPRISE_OID; // Set enterprise OID
	s32_t generic_trap = 6;  // Generic trap code (e.g., 6 for enterprise specific)
	s32_t specific_trap = SNMP_GENTRAP_COLDSTART + 1;  // +1 to make it 1-based
/*
 * Specific trap code (customize as needed):
 *     1. coldStart
 *     2. warmStart
 *     3. linkDown
 *     4. linkUp
 *     5. authentication error
 * The chosen digit will be appended to '1.3.6.1.6.3.1.1.5'.
 */
	// Allocate memory for varbinds, about 220 bytes
	// It will allocate SNMP_MAX_OBJ_ID_LEN (50) uint32's.
	varbinds = (struct snmp_varbind *)mem_malloc(sizeof(struct snmp_varbind));

	if (varbinds != NULL) {
		int index;
		// Set up varbinds
		varbinds->oid.len = 8;// Length of OID

		varbinds->oid.id[0] = 1; // See https://oid-base.com/get/1.3.6.1.2.1.1.4
		varbinds->oid.id[1] = 3;
		varbinds->oid.id[2] = 6;
		varbinds->oid.id[3] = 1;
		varbinds->oid.id[4] = 2;
		varbinds->oid.id[5] = 1;
		varbinds->oid.id[6] = 1;
		varbinds->oid.id[7] = 4;

		varbinds->type = SNMP_ASN1_TYPE_UNSIGNED32; // Type of the variable
		u32_t value = 100;                          // Example value to send
		varbinds->value = (void *)&value;           // Pointer to value
		varbinds->value_len = sizeof(value);        // Length of value

		err_t err = snmp_send_trap(&enterprise_oid, generic_trap, specific_trap, varbinds);
		
		if (err == ERR_OK) {
			zephyr_log("SNMP trap sent successfully.\n");
		} else {
			zephyr_log("Failed to send SNMP trap: %d\n", err);
		}

		// Free allocated memory for varbinds
		mem_free(varbinds);
	} else {
		zephyr_log("Memory allocation failed for varbinds.\n");
	}
}

K_THREAD_DEFINE(z_snmp, STACKSIZE, z_snmp_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
