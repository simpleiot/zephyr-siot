# Zephyr SimpleIot Application

This is a Zephyr Workspace
[T2 topology](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository)
that sets up to build the SIOT Zephyr application.

## Setup

- `mkdir siot-work`
- `cd siot-work`
- `west init -m https://github.com/simpleiot/zephyr-siot.git`
- `west update`
- `cd siot`
- `siot_setup`

## Build

- `cd siot-work/siot`
- `. envsetup.sh` (notice leading `.`)
- `siot_build_esp32_poe` (there are also other build targets)
- `siot_flash <serial port>` (serial port is required for ESP targets)
