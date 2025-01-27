#ifndef BLE_H
#define BLE_H

#include <stdbool.h>

// Function declarations
void ble_init(void);
void ble_start_advertising(void);
bool ble_is_connected(void);
void ble_send_pin_status(void);

#endif /* BLE_H */ 