#include "point.h"

#include "zephyr/kernel.h"
#include <zephyr/net/http/server.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <sys/types.h>
#include <app_version.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

ZBUS_CHAN_DEFINE(siot_point_chan, point, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(z_ticker_chan, uint8_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

// ==================================================
// Network manager

static struct net_mgmt_event_callback mgmt_cb;

static void net_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			      struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected\n");
	}
}

void z_ticker_callback(struct k_timer *timer_id)
{
	uint8_t dummy = 0;
	zbus_chan_pub(&z_ticker_chan, &dummy, K_MSEC(500));
}

K_TIMER_DEFINE(z_ticker, z_ticker_callback, NULL);

// ********************************
// main

int main(void)
{
	LOG_INF("Zonit M+R: %s %s", CONFIG_BOARD_TARGET, APP_VERSION_EXTENDED_STRING);

	k_timer_start(&z_ticker, K_MSEC(500), K_MSEC(500));

	// In your initialization code:
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);
}
