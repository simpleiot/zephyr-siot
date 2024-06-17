# SIOT_BOARD=esp32_devkitc_wroom
SIOT_BOARD=esp32_poe/esp32/procpu

siot_setup() {
	west blobs fetch hal_espressif
}

siot_build() {
	west build -b $SIOT_BOARD apps/siot -- -DBOARD_ROOT="$(pwd)"
}

siot_flash() {
	PORT=$1
	if [ "$PORT" = "" ]; then
		echo "serial port must be provided"
		return 1
	fi
	west flash --esp-device="$PORT"
}

siot_flash_cliff() {
	siot_flash /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
}
