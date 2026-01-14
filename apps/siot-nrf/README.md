# SIOT nRF

The SIOT nRF application runs on Nordic nRF cellular modules such as the nRF9151
Feather.

## Building/Flashing

- `siot_build_nrf9151_feather apps/siot-nrf`
- `siot_flash_nrf`
- open serial console: `tio /dev/serial/by-id/usb-Raspberry_Pi_Debug_Probe...`

## Using

- `lte normal`: connect device to cellular network
- `lte offline`: disconnect device from cellular
