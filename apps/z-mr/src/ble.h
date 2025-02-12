/**
 * @file ble.h
 * @brief Bluetooth Low Energy interface
 *
 * This header provides the public interface for the BLE functionality
 */

#ifndef BLE_H_
#define BLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the BLE subsystem
 *
 * This function initializes Bluetooth, starts advertising, and sets up all required
 * services including vendor-specific service, Heart Rate Service (HRS),
 * Battery Service (BAS), and Current Time Service (CTS).
 *
 * @return 0 on success, negative errno on failure
 */
int ble_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_H_ */
