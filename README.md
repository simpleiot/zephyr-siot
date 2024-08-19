# Zephyr SimpleIot Application

This is a Zephyr Workspace
[T2 topology](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository)
that sets up to build the SIOT Zephyr application.

## Setup

- `mkdir zephyr-siot`
- `cd zephyr-siot`
- `west init -m https://github.com/simpleiot/zephyr-siot.git`
- `west update`
- `cd siot`
- `. envsetup.sh` (notice leading `.`)
- `siot_setup`

## Build

- `cd zephyr-siot/siot`
- `. envsetup.sh` (notice leading `.`)
- `siot_build_<board> src/<app>`

## Flashing

- `siot_flash (STM32)`
- `siot_flash_esp <serial port>` (serial port is required for ESP targets)
- `tio /dev/serial/by-id/...`

## Applications

- **[siot-net](apps/siot-net)**: Start of generic network connected SIOT app
- **[siot-serial](apps/siot-serial)**: Start of generic serial connected SIOT
  app
- **[siggen](apps/siggen)**: Signal generator app that uses DAC output.
