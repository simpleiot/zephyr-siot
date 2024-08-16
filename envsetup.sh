siot_setup() {
	west blobs fetch hal_espressif
}

# APP is one of directories in apps/
siot_build() {
	BOARD=$1
	APP=$2
	# see if we need to tack on board root
	if grep -q -E "(esp32_poe)" <<<"${BOARD}"; then
		echo "using custom board"
		west build -b "${BOARD}" "${APP}" -- -DBOARD_ROOT="$(pwd)"
	else
		echo "using zephyr board"
		west build -b "${BOARD}" "${APP}"
	fi
}

# https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/open-source-hardware
siot_build_esp32_poe() {
	APP=$1
	siot_build esp32_poe/esp32/procpu "${APP}"
	#west build -b esp32_poe/esp32/procpu apps/siot -- -DBOARD_ROOT="$(pwd)"
}

# https://docs.zephyrproject.org/latest/boards/espressif/esp32_ethernet_kit/doc/index.html
siot_build_esp32_ethernet_kit() {
	APP=$1
	siot_build esp32_ethernet_kit/esp32/procpu "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/st/nucleo_h743zi/doc/index.html
siot_build_nucleo_h743zi() {
	APP=$1
	siot_build nucleo_h743zi "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/st/nucleo_l452re/doc/index.html
siot_build_nucleo_l452re() {
	APP=$1
	siot_build nucleo_l452re "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/st/nucleo_l432kc/doc/index.html
siot_build_nucleo_l432kc() {
	APP=$1
	siot_build nucleo_l432kc "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/espressif/esp32_devkitc_wroom/doc/index.html
siot_build_esp32_wroom() {
	APP=$1
	siot_build esp32_devkitc_wroom/esp32/procpu "${APP}"
}

# following can be used for STM32 targets + Jlink
siot_flash() {
	west flash
}

# used to flash ESP targets where serial port must be provided
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
