# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(siot)

zephyr_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_sources(app PRIVATE 
	src/main.c 
	src/led.c
	src/w1.c
	src/web.c
	src/dc.c
	src/fan.c
	src/ats.c
	# src/snmp.c
	# src/ble.c
)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(web_resource index.html index.js)
	generate_inc_file_for_target(
		app
		frontend/dist/${web_resource}
		${gen_dir}/${web_resource}.gz.inc
		--gzip
	)
endforeach()

zephyr_linker_sources(SECTIONS sections-rom.ld)

zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE NAME
				http_resource_desc_test_http_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN Z_LINK_ITERABLE_SUBALIGN)

