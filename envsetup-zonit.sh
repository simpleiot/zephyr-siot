. envsetup.sh

# Zonit specific build functions

z_mr_build() {
	z_mr_frontend_build
	siot_build_esp32_poe apps/z-mr
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
	(
		cd apps/z-mr/frontend &&
			(
				elm-land build &&
					mv dist/assets/index*.js dist/ &&
					for file in dist/index-*.js; do mv "$file" "${file/index-*./index.}"; done &&
					sed -i 's/assets\/index.*\.js/index.js/g' dist/index.html ||
					return 1
			)
	)
}
