siot_setup() {
	west blobs fetch hal_espressif
}

siot_build_esp32_poe() {
	west build -b esp32_poe/esp32/procpu apps/siot -- -DBOARD_ROOT="$(pwd)"
}

siot_build_esp32_ethernet_kit() {
	west build -b esp32_ethernet_kit/esp32/procpu apps/siot
}

siot_build_nucleo_h743zi() {
	west build -b nucleo_h743zi apps/siot
}

siot_build_esp32_wroom() {
	west build -b esp32_devkitc_wroom/esp32/procpu apps/siot
}

# following can be used for STM32 targets + Jlink
siot_flash() {
	west flash
}

siot_flash_esp() {
	PORT=$1
	if [ "$PORT" = "" ]; then
		echo "serial port must be provided"
		return 1
	fi
	west flash --esp-device="$PORT"
}

siot_flash_esp_cliff() {
	siot_flash_esp /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
}

siot_peek_generated_confg() {
	$EDITOR build/zephyr/include/generated/zephyr/autoconf.h
}

siot_peek_generated_dts() {
	$EDITOR build/zephyr/zephyr.dts
}

siot_clean() {
	rm -rf build
}

siot_menuconfig() {
	west build -t menuconfig
}

GENERATED_DEFCONFIG=build/zephyr/kconfig/defconfig
SAVED_DEFCONFIG=apps/siot/defconfig

siot_defconfig_diff() {
	nvim -d $GENERATED_DEFCONFIG $SAVED_DEFCONFIG
}

siot_defconfig_save() {
	cp $GENERATED_DEFCONFIG $SAVED_DEFCONFIG
}
