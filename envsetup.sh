siot_setup() {
	west update
	west blobs fetch hal_espressif
}

siot_net_frontend_watch() {
	# TARGET_IP is not working yet in elm-land.json
	# TARGET_IP=$1
	# if [ "$TARGET_IP" = "" ]; then
	# 	echo "target IP must be provided"
	# 	return 1
	# fi

	# export TARGET_IP=$TARGET_IP
	(cd apps/siot-net/frontend && elm-land server)
}

siot_net_frontend_build() {
	(
		cd apps/siot-net/frontend &&
			(
				elm-land build &&
					mv dist/assets/index*.js dist/ &&
					for file in dist/index-*.js; do mv "$file" "${file/index-*./index.}"; done &&
					sed -i 's/assets\/index-[A-Za-z0-9]\+-\.js/index.js/g' dist/index.html ||
					return 1
			)
	)
}

siot_build_native_sim() {
	APP=$1
	west build -b native_sim "${APP}"
}

# run all library tests on host platform
siot_test_native() {
	siot_build_native_sim tests && ./build/zephyr/zephyr.exe
}

# See https://community.tmpdir.org/t/zephyr-on-the-esp32/1310 for a comparison of ESP hardware

# https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/open-source-hardware
siot_build_esp32_poe() {
	APP=$1
	west build -b esp32_poe/esp32/procpu "${APP}" -- -DBOARD_ROOT="$(pwd)"
}

siot_build_fysetc_ucan() {
	APP=$1
	west build -b fysetc_ucan "${APP}" -- -DBOARD_ROOT="$(pwd)"
}

# https://docs.zephyrproject.org/latest/boards/espressif/esp32_ethernet_kit/doc/index.html
siot_build_esp32_ethernet_kit() {
	APP=$1
	west build -b esp32_ethernet_kit/esp32/procpu "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/espressif/esp32c3_devkitc/doc/index.html
# DevKitC is the larger module
siot_build_esp32c3_devkitc() {
	APP=$1
	west build -b esp32c3_devkitc "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/espressif/esp32c3_devkitm/doc/index.html
# DevKitM is the mini (smaller module)
siot_build_esp32c3_devkitm() {
	APP=$1
	west build -b esp32c3_devkitm "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/st/nucleo_h743zi/doc/index.html
siot_build_nucleo_h743zi() {
	APP=$1
	west build -b nucleo_h743zi "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/st/nucleo_l452re/doc/index.html
siot_build_nucleo_l452re() {
	APP=$1
	west build -b nucleo_l452re "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/st/nucleo_l432kc/doc/index.html
siot_build_nucleo_l432kc() {
	APP=$1
	west build -b nucleo_l432kc "${APP}"
}

# https://docs.zephyrproject.org/latest/boards/espressif/esp32_devkitc_wroom/doc/index.html
siot_build_esp32_wroom() {
	APP=$1
	west build -b esp32_devkitc_wroom/esp32/procpu "${APP}"
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
