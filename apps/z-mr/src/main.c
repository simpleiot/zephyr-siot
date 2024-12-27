
#include "ats.h"
#include "config.h"
#include "point.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/zbus/zbus.h>
#include <app_version.h>

LOG_MODULE_REGISTER(z_mr, LOG_LEVEL_DBG);

ZBUS_OBS_DECLARE(z_config_sub);

ZBUS_CHAN_DEFINE(z_temp_chan, float, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(z_ats_chan, ats_state, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.state = {
				       {false, false, false, false},
				       {false, false, false, false},
				       {false, false, false, false},
				       {false, false, false, false},
				       {false, false, false, false},
				       {false, false, false, false},
			       }));
ZBUS_CHAN_DEFINE(z_config_chan, z_mr_config, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(z_point_chan, point, NULL, NULL, ZBUS_OBSERVERS(z_config_sub), ZBUS_MSG_INIT(0));

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

int main(void)
{
	LOG_INF("Zonit M+R: %s %s", CONFIG_BOARD_TARGET, APP_VERSION_EXTENDED_STRING);

	// In your initialization code:
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);
}
