#include "zpoint.h"

#include <nvs.h>
#include <point.h>

#include <stdint.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/device.h>
#include <app_version.h>

LOG_MODULE_REGISTER(z_mr, LOG_LEVEL_DBG);

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

int main(void)
{
	LOG_INF("Zonit M+R: %s %s", CONFIG_BOARD_TARGET, APP_VERSION_EXTENDED_STRING);

	nvs_init(nvs_pts, ARRAY_SIZE(nvs_pts));

	// In your initialization code:
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);

	const struct device *eeprom = DEVICE_DT_GET(DT_NODELABEL(m24512));
	uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
	int rc;

	if (!device_is_ready(eeprom)) {
		LOG_ERR("EEPROM device not ready\n");
		return 0;
	}

	int size = eeprom_get_size(eeprom);
	LOG_DBG("EEPROM size: %i", size);

	// Write data to EEPROM
	LOG_DBG("Write to eeprom");
	rc = eeprom_write(eeprom, 0, data, sizeof(data));
	if (rc < 0) {
		LOG_ERR("Failed to write to EEPROM\n");
		return 0;
	}

	// Read data from EEPROM
	LOG_DBG("read eeprom");
	uint8_t read_data[4];
	rc = eeprom_read(eeprom, 0, read_data, sizeof(read_data));
	if (rc < 0) {
		LOG_ERR("Failed to read from EEPROM\n");
		return 0;
	}

	LOG_INF("Read data: %02x %02x %02x %02x\n", read_data[0], read_data[1], read_data[2],
		read_data[3]);
}
