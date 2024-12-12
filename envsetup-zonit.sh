. envsetup.sh

# Zonit specific build functions

z_build_zmr() {
	siot_build_esp32_poe apps/z-mr
}

z_flash_zmr() {
	siot_flash_esp "$1"
}
