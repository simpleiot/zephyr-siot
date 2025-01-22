# Zonit M+R Zephyr Firmware

- [Product documentation](https://gitea.zonit.com/Zonit-Dev/product/src/branch/master/z-mr)

This application is meant to run on the ESP32-POE board from Olimex.

## Building/Flashing

In repo root directory:

- see top level
  [README-Zonit.md](https://gitea.zonit.com/Zonit-Dev/zephyr-zonit/src/branch/main/README-Zonit.md)
  for instructions on workspace setup.
- `. envsetup-zonit.sh`
- `z_build_zmr`
- `z_flash_zmr <serial port>`

## Application architecture

The application is constructed of a number of threads that mostly read or write
points to a single
[zbus](https://docs.zephyrproject.org/latest/services/zbus/index.html) channel.
For example, the 1-wire thread may read a temperature sensor and publish a
temperature point. The web thread may publish this point to the web UI frontend
for display to the user. The fan control thread may also listen for temp points
and use them to control the fan speed. This architecture largely decouples
threads from each other. The 1-wire thread does not need to know the web and fan
control threads exist. The point type is what is used to determine where and how
data is used.

A point is a struct with the following fields:

| Field       | Type      | Description                                                                |
| ----------- | --------- | -------------------------------------------------------------------------- |
| `time`      | `uint64`  | nanoseconds since Unix epoch                                               |
| `type`      | `char[]`  | Point type                                                                 |
| `key`       | `char[]`  | Point key -- can be used for index to create arrays, or key to create maps |
| `data_type` | `uint8`   | Encoding of data field (currently float, int, or string)                   |
| `data`      | `uint8[]` | Data payload for point                                                     |

## Storing settings in flash

The Zephyr
[NVS subsystem](https://docs.zephyrproject.org/latest/services/storage/nvs/nvs.html)
is used to store settings in flash. If you want a point to be persisted in
flash, then add it to the table in `nvs_pts` table in `main.c`. This causes the
NVS thread to listen for this point type and store it in flash any time it sees
it. On boot, it will also read these points from flash and send them on the
points channel to configure the system with the saved settings.

## Web UI Frontend

The web UI for the project is a single-page application (SPA) written in Elm
that reads and writes JSON data to an HTTP API implemented in the Zephyr `web.c`
module.

[elm-land](https://elm.land/) comes with a development server that runs on your
workstation and updates the web page whenever it changes without losing state.
This is useful because you can do all of the web frontend develoment without
reflashing the Z-MR.

To run the development server: `z_mr_frontend_watch`

You will need to update the proxy field in `apps/z-mr/frontend/elm-land.json` to
match the IP address of your Z-MR target system. (long term we'd like this to be
an environment variable, but that is not working yet)

## Upgrading/Flashing without the Zephyr build system

This can be used in manufacturing, field updates, etc.

- Make sure
  [esptool](https://docs.espressif.com/projects/esptool/en/latest/esp32/) is
  installed on your laptop.

- `pip install esptool`

and test it by asking for it's ID:

- `esptool.py -p <serial port> flash_id`

where `<serial port>` will normally be `/dev/ttyUSB0`

Use the following command line to upgrade the ESP32:

`esptool.py --chip auto --port <serial port> --baud 921600 --before default_reset --after hard_reset write_flash -u --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 <bin file>`

where `<bin file>` is the binary image of the flash.

## Operation

- open a serial console on the USB serial port
- connect an Ethernet cable
- the device will get an IP address via DHCP and display it in the serial
  console
- open a web page on the device IP address to see status
  http://device-ip-address/ -- example show below:

<img src="assets/image-20241205135158112.png" alt="image-20241205135158112" style="zoom:50%;" />

## Olimex Pin Mapping

![Olimex pins](assets/olimex-pins.png)

![DC Wiring](assets/z-ind-monitor-matrix-map.png)

### I2C Devices

- DS2484R+T: `0x18`
- IC1: MCP23018T-E/SO: `0x20` (up to 8 IO-expanders can be added from `0x20` to
  `0x27`)
- EMC2302-1-AIZL: `0x2E`
- 8K EEProm: `0x50` to `0x57`
- RV-3028-C7 RTC: `0x82`

## SNMP

We are planning to port the
[LWIP SNMP](https://www.nongnu.org/lwip/2_1_x/group__snmp.html) code to Zephyr
([source code](https://github.com/lwip-tcpip/lwip/tree/master/src/apps/snmp)).
See
[this dicussion thread](https://github.com/zephyrproject-rtos/zephyr/discussions/80648).

The initial goal is to support SNMPv2c.

A public Zephyr module has been set up in this repo:

https://github.com/simpleiot/zephyr-snmp

This module is included in our build by adding it to
[`west.yml`](../../west.yml).

The SNMP library can be enabled by adding the following to `prj.conf`:

`CONFIG_LIB_SNMP=y`

It still needs ported and currently does not compile.

### SNMP Testing

The Industrial monitor applications sends SNMP Traps when events occur.

#### SNMP Test Server

##### net-snmp

Populate `/etc/snmp/snmptrapd.conf` with the following:

```
authCommunity log,execute,net public
disableAuthorization yes
```

`sudo snmptrapd -f -Lo`

##### Telegraf

This can be tested using
[Telegraf](https://www.influxdata.com/time-series-platform/telegraf/).

Steps:

- install Telegraf
- SNMP listens on port 162 by default, which is a priveleged port. On Linux you
  can do something like:
  `sudo setcap cap_net_bind_service=+ep /usr/bin/telegraf`
- run telegraf, run the following from this directory:
  `telegraf  --config test/telegraf.conf`

Now, Telegraf will print out any data it receives to `stdout`.

#### Send a test trap

You can test a trap by:

- install: `net-snmp`
- run:
  `snmptrap -v 2c -c public localhost '' NET-SNMP-EXAMPLES-MIB::netSnmpExampleHeartbeatNotification netSnmpExampleHeartbeatRate i 123456`
