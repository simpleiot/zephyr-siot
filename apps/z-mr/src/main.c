#include "zpoint.h"

#include <nvs.h>
#include <point.h>
// #include <ble.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <app_version.h>

LOG_MODULE_REGISTER(z_mr, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

const point_def point_def_snmp_server = {POINT_TYPE_SNMP_SERVER, POINT_DATA_TYPE_STRING};

// The following points will get persisted in NVS when the show up on
// zbus point_chan.
static const struct nvs_point nvs_pts[] = {
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

static void input_callback(struct input_event *evt, void *user_data)
{
	if (evt->type == INPUT_EV_KEY) {
		if (evt->code == INPUT_KEY_0) {
			point p;
			point_set_type_key(&p, POINT_TYPE_SWITCH, "0");
			point_put_int(&p, evt->value);
			zbus_chan_pub(&point_chan, &p, K_MSEC(500));
		}
	}
}

INPUT_CALLBACK_DEFINE(NULL, input_callback, NULL);

int main(void)
{
	LOG_INF("Zonit Z-MR: %s %s", CONFIG_BOARD_TARGET, APP_VERSION_EXTENDED_STRING);

	nvs_init(nvs_pts, ARRAY_SIZE(nvs_pts));

	// ble_init();

	// In your initialization code:
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);
}
