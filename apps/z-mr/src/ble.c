#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include "ble.h"

LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

#define SERVICE_UUID_VAL \
    BT_UUID_128_ENCODE(0x4fafc201, 0x1fb5, 0x459e, 0x8fcc, 0xc5c9c331914bULL)
#define CHAR_UUID_VAL \
    BT_UUID_128_ENCODE(0xbeb5483e, 0x36e1, 0x4688, 0xb7f5, 0xea07361b26a8ULL)

static struct bt_conn *current_conn;
static bool device_connected;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, SERVICE_UUID_VAL),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    LOG_INF("Connected");
    current_conn = bt_conn_ref(conn);
    device_connected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)", reason);

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
    int err;

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    bt_conn_cb_register(&conn_callbacks);
    LOG_INF("Bluetooth initialized");
    
    // Start advertising immediately
    ble_start_advertising();
}

void ble_start_advertising(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising started");
}

bool ble_is_connected(void)
{
    return device_connected;
}

void ble_send_pin_status(void)
{
    // This function will need to be implemented based on your specific needs
    // You'll need to create a GATT service and characteristic to send the data
    if (device_connected) {
        // Send pin status via BLE notification
        // Implementation depends on your GATT service structure
    }
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        ble_start_advertising();
        delay(200);
    }
    
    ble_send_pin_status();
    delay(50);
} 