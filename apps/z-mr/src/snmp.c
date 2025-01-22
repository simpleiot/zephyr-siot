#include "zpoint.h"
#include <point.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/zbus/zbus.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(z_snmp, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);
ZBUS_CHAN_DECLARE(ticker_chan);

ZBUS_MSG_SUBSCRIBER_DEFINE(z_snmp_sub);
ZBUS_CHAN_ADD_OBS(point_chan, z_snmp_sub, 3);
ZBUS_CHAN_ADD_OBS(ticker_chan, z_snmp_sub, 4);

void z_snmp_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("z snmp thread");

	point p;
	const struct zbus_channel *chan;

	while (!zbus_sub_wait_msg(&z_snmp_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {
			if (strcmp(p.type, POINT_TYPE_ATS_A) == 0) {
				// TODO: eventually, send ATS event out SNMP after we define MIB
			} else if (strcmp(p.type, POINT_TYPE_ATS_B) == 0) {
				// TODO: eventually, send ATS event out SNMP after we define MIB
			}
		} else if (chan == &ticker_chan) {
			// can do periodic stuff here
			// TODO: send out test SNMP trap, call function in SNMP lib
			// ip_address = "192.168.1.23"   // IP address of test PC running net-snmp
			// trap_def = <define some test snmp message>
			// snmp_send_trap(&trap_def, &ip_address);
		}
	}
}

K_THREAD_DEFINE(z_snmp, STACKSIZE, z_snmp_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
