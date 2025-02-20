#include "zpoint.h"

#include <nvs.h>
#include <point.h>
// #include <ble.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
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
