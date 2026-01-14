# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(siot-nrf-simple)

# If board is nRF9160 add the source files 
target_sources(app PRIVATE src/main.c)



