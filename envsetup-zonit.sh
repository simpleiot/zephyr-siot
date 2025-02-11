# rationale for envsetup.sh:
# https://daily.bec-systems.com/0097-envsetup-sh-a-very-useful-automation-technique/

. ./envsetup.sh

# Zonit specific build functions

z_mr_build() {
	z_mr_frontend_build || return 1
	siot_build_esp32_poe apps/z-mr
}

z_mr_build_wrover() {
	z_mr_frontend_build || return 1
	siot_build_esp32_poe_wrover apps/z-mr
}

z_mr_flash() {
	siot_flash_esp "$1"
}

z_mr_frontend_watch() {
	# TARGET_IP is not working yet in elm-land.json
	# TARGET_IP=$1
	# if [ "$TARGET_IP" = "" ]; then
	# 	echo "target IP must be provided"
	# 	return 1
	# fi

	# export TARGET_IP=$TARGET_IP
	(cd apps/z-mr/frontend && elm-land server)
}

z_mr_frontend_build() {
	# elm-land >/dev/null 2>&1 || { echo "Error: $1 is not installed." >&2; exit 1; }
	if ! elm >/dev/null 2>&1; then
		echo "Please install elm: npm install -g elm@latest"
		return 1
	fi
	if ! elm-land >/dev/null 2>&1; then
		echo "Please install elm-land: npm install -g elm-land@latest"
		return 1
	fi
	(
		cd apps/z-mr/frontend &&
			(
				elm-land build &&
					mv dist/assets/index*.js dist/ &&
					for file in dist/index-*.js; do mv "$file" "${file/index-*./index.}"; done &&
					if [[ "$OSTYPE" == "darwin"* ]]; then
						# macOS version (BSD sed)
						sed -i '' -e 's|assets/index[^/]*\.js|index.js|g' dist/index.html
					else
						# Linux version (GNU sed)
						sed -i 's|assets/index[^/]*\.js|index.js|g' dist/index.html
					fi ||
					return 1
			)
	)
}

z_mr_snmptrapd() {
	echo About to run snmptrapd with sudo. It will show all SNMP traps received.
	echo You can test it by calling the z_mr_snmptrapd\(\) function.
	sudo snmptrapd -f -Loe
}

z_mr_traptest() {
	echo This command will send a test trap message from and to this host
	sudo snmptrap -v 2c -c public localhost '' SNMPv2-MIB::coldStart
}

