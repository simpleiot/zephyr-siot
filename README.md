# Zephyr SimpleIot Application

This is a Zephyr Workspace
[T2 topology](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository)
that sets up to build the SIOT Zephyr application.

## Setup

- install Zephyr dependencies as outlined in the
  [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#)
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

## Open serial console

The SIOT firmware applications all have the serial shell enabled, so you can
connect to that to interact with the system.

Highly recommend the [tio](https://github.com/tio/tio) tool.

- `tio /dev/serial/by-id/...`

## Applications

- **[siot-net](apps/siot-net)**: Start of generic network connected SIOT app
- **[siot-serial](apps/siot-serial)**: Start of generic serial connected SIOT
  app
- **[siggen](apps/siggen)**: Signal generator app that uses DAC output.

## Memory management

Getting all the memory knobs tuned correctly in Zephyr can be a challenge.
Generally, the advice is to make sure stacks never go above 70% used
(`kernel thread stacks`).

The following config options can also be enabled to help track down memory
issues:

```
# Stack debugging information
# TODO: may want to remove for production
CONFIG_ASSERT=y
CONFIG_ASSERT_VERBOSE=y
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_STACK_CANARIES=y
CONFIG_STACK_SENTINEL=y
```
