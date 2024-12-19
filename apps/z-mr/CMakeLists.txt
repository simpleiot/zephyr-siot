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
	src/config.c
	src/point.c
	src/fan.c
	${CMAKE_CURRENT_SOURCE_DIR}/../common/html.c
)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(source_file_index src/index.html)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.gz.inc --gzip)

zephyr_linker_sources(SECTIONS sections-rom.ld)

zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE NAME
				http_resource_desc_test_http_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN Z_LINK_ITERABLE_SUBALIGN)

