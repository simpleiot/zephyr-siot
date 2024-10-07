. envsetup.sh

# Zonit specific build functions

z_build_industrial() {
	siot_build_esp32_poe apps/industrial
}

z_flash_industrial() {
	siot_flash_esp "$1"
}
