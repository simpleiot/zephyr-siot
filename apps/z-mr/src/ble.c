#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include "ble.h"

LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

#define SERVICE_UUID_VAL BT_UUID_128_ENCODE(0x4fafc201, 0x1fb5, 0x459e, 0x8fcc, 0xc5c9c331914b)
#define CHAR_UUID_VAL    BT_UUID_128_ENCODE(0xbeb5483e, 0x36e1, 0x4688, 0xb7f5, 0xea07361b26a8)

// Define characteristic UUIDs for different data types
#define SYSTEM_STATUS_UUID_VAL BT_UUID_128_ENCODE(0xbeb5483e, 0x36e1, 4688, 0xb7f5, 0xea07361b26a8)
#define ATS_STATUS_UUID_VAL    BT_UUID_128_ENCODE(0xbeb5483e, 0x36e1, 4689, 0xb7f5, 0xea07361b26a8)
#define SETTINGS_UUID_VAL      BT_UUID_128_ENCODE(0xbeb5483e, 0x36e1, 4690, 0xb7f5, 0xea07361b26a8)

// Buffers for each characteristic
static uint8_t system_status_buffer[256];
static uint8_t ats_status_buffer[256];
static uint8_t settings_buffer[512];

// CCC configurations
static struct bt_gatt_ccc_cfg system_status_ccc_cfg[BT_GATT_CCC_MAX] = {};
static struct bt_gatt_ccc_cfg ats_status_ccc_cfg[BT_GATT_CCC_MAX] = {};

static struct bt_conn *current_conn;
static bool device_connected;
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SERVICE_UUID_VAL),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (%u)", err);
		return;
	}
	current_conn = bt_conn_ref(conn);
	device_connected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		device_connected = false;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void ble_init(void)
{
	if (bt_enable(NULL)) {
		return;
	}
	bt_conn_cb_register(&conn_callbacks);
	ble_start_advertising();
}

void ble_start_advertising(void)
{
	bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
}

bool ble_is_connected(void)
{
	return device_connected;
}

// Remove ble_send_pin_status() if not implemented/needed

// Callback handlers
static void status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notifications = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Status notifications %s", notifications ? "enabled" : "disabled");
}

// Read handlers
static ssize_t read_system_status(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	k_mutex_lock(&web_points_lock, K_FOREVER);

	// Filter and encode only system status points
	point status_points[5];
	int count = 0;

	for (int i = 0; i < ARRAY_SIZE(web_points); i++) {
		if (Point.isSystemPoint(&web_points[i])) { // You'll need to implement this
			status_points[count++] = web_points[i];
		}
	}

	int ret = points_json_encode(status_points, count, system_status_buffer,
				     sizeof(system_status_buffer));
	k_mutex_unlock(&web_points_lock);

	if (ret < 0) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, system_status_buffer,
				 strlen(system_status_buffer));
}

static ssize_t read_ats_status(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	k_mutex_lock(&web_points_lock, K_FOREVER);

	// Filter and encode only ATS-related points
	point ats_points[12]; // 6 for side A, 6 for side B
	int count = 0;

	for (int i = 0; i < ARRAY_SIZE(web_points); i++) {
		if (Point.isAtsPoint(&web_points[i])) { // You'll need to implement this
			ats_points[count++] = web_points[i];
		}
	}

	int ret =
		points_json_encode(ats_points, count, ats_status_buffer, sizeof(ats_status_buffer));
	k_mutex_unlock(&web_points_lock);

	if (ret < 0) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ats_status_buffer,
				 strlen(ats_status_buffer));
}

// Write handler for settings
static ssize_t write_settings(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	point pts[5] = {};
	int ret = points_json_decode(buf, len, pts, ARRAY_SIZE(pts));

	if (ret < 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	k_mutex_lock(&web_points_lock, K_FOREVER);
	for (int i = 0; i < ret; i++) {
		if (Point.isSettingsPoint(&pts[i])) { // You'll need to implement this
			points_merge(web_points, ARRAY_SIZE(web_points), &pts[i]);
			zbus_chan_pub(&point_chan, &pts[i], K_MSEC(500));
		}
	}
	k_mutex_unlock(&web_points_lock);

	return len;
}

// GATT service definition
BT_GATT_SERVICE_DEFINE(zmr_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(SERVICE_UUID_VAL)),

		       // System Status characteristic
		       BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(SYSTEM_STATUS_UUID_VAL),
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_READ, read_system_status, NULL, NULL),
		       BT_GATT_CCC(system_status_ccc_cfg, status_ccc_cfg_changed),

		       // ATS Status characteristic
		       BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(ATS_STATUS_UUID_VAL),
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_READ, read_ats_status, NULL, NULL),
		       BT_GATT_CCC(ats_status_ccc_cfg, status_ccc_cfg_changed),

		       // Settings characteristic
		       BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(SETTINGS_UUID_VAL),
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					      read_system_status, write_settings, NULL), );
