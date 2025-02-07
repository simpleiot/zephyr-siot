#include "zpoint.h"

#include <nvs.h>
#include <point.h>

#include <stdint.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/zbus/zbus.h>
#include <app_version.h>

LOG_MODULE_REGISTER(z_mr, LOG_LEVEL_DBG);

const RTC_RODATA_ATTR point_def point_def_snmp_server Z_GENERIC_SECTION(.rodata) = {POINT_TYPE_SNMP_SERVER, POINT_DATA_TYPE_STRING};

// The following points will get persisted in NVS when the show up on
// zbus point_chan.
static const RTC_RODATA_ATTR struct nvs_point nvs_pts[] Z_GENERIC_SECTION(.rodata) = {
	{1, &point_def_boot_count, "0"},  {2, &point_def_description, "0"},
	{3, &point_def_staticip, "0"},    {4, &point_def_address, "0"},
	{5, &point_def_gateway, "0"},     {6, &point_def_netmask, "0"},
	{7, &point_def_snmp_server, "0"},
};

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

	nvs_init(nvs_pts, ARRAY_SIZE(nvs_pts));

	// In your initialization code:
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);
}
