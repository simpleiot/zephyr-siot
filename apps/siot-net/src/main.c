
#include <zephyr/net/http/server.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <sys/types.h>
#include <app_version.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

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

// ********************************
// main

int main(void)
{
	LOG_INF("Zonit M+R: %s %s", CONFIG_BOARD_TARGET, APP_VERSION_EXTENDED_STRING);

	// In your initialization code:
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);
}
