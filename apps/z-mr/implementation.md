# Z-MR Implementation Plan

Below is a rough order/outline for implementing the Z-MR functionality

## SNMP

1. understand SNMP
   - [SNMP Protocol Architecture – MIBs and OIDs explained](https://www.itprc.com/snmp-protocol-architecture-mibs-oids/)
1. (DONE) get net-snmp tools running on PC
1. (DONE) implement sending a test test trap from Z-MR
   - Z-MR should call a function in zephyr-snmp to send a trap. The trap
    definition and IP address should be passed to this function.
1. (DONE) receive test trap on PC using net-snmp and write envsetup-zonit.sh script to
  document
1. UI to configure SNMP server IP address to receive traps
   - update UI with SNMP server IP address field
   - persist this setting in flash
   - use this point to set the IP address when sending traps
1. research existing OIDs to see if ATS OIDs/MIBs existing
1. implement custom OID/MIB if necessary
1. implement SNMP GetRequest
   - zephyr-snmp lib should listen for requests on SNMP port
   - zephyr-snmp implements a function that allows you to register a callback to
    receive requests
     - this can be modeled after the Zephyr http lib API (see web.c for how we
      handle http reqeusts).
   - Z-MR snmp.c thread should register a callback with zephyr-snmp lib and
     handle requests
1. handle basic SNMP requests such as uptime or whatever else is normal for a
   system to report
1. figure out how to report ATS state
   - are there existing MIBS that cover this?
   - do we need to design a custom one?
1. automatic testing
   - we should have
    [Zephyr tests](https://docs.zephyrproject.org/latest/develop/test/ztest.html)
    for zephyr-snmp and Z-MR snmp functionality if possible
   - see the `tests` directory for an example of tests already implemented

## Bluetooth Configuration

The goal will be to use the same Z-MR web UI and send point data over bluetooth
instead of HTTP requests (Ethernet). The BT connection will be able to display
everything the web connection will, with the exception of delivering the inital
page.

- get Bluetooth working on Z-MR under Zephyr and responding to some request
  - add ble thread
- implement a web-bt javascript module in the Z-MR web UI to connect with the
  Z-MR
  - may need to embed this module in the image, similar to index.html and
    index.js
- implement an ELM UI to display discovered devices and select one. This will
  communicate with JS code over ports.
- research if we can stream points over a single UUID using BLE, or if we will
  need a separate UUID for every point.
- frontend: create a Elm port to receive points and update local state
- cache points in ble thread and send to web UI on BT connection. Points are
  sent to Elm through port.
- create another Elm port to send any modified points to the BLE JS code. If
  there is a connection active, these points get written over BLE.
- only start BLE after button press and shut off after 15m

## Serial

Goal is the same as the Bluetooth -- the web UI will display the same
information as Ethernet connection. We will leverage the existing terminal and
parse terminal activity to read and set points.

- add command to dump points to serial terminal (we may have a command to
  enable/disable this so it is not noisy during development)
- add a terminal command to send points
- create JS code in Z-MR web UI to open a serial port and read the terminal data
- create an ELM UI to configure the serial connection and communicate over ports
  to the JS code.
- once connection is made, the serial code will listen to the same Elm ports as
  BT to receive and send points.
- BONUS: display raw terminal traffic in ELM web UI (perhaps a diagnostic page)
- BONUS^2: accept terminal commands in ELM web UI terminal display

## PWA

- figure out and document setup for development (requires HTTPS)
- create manifest.json and embed in image
- add service worker if necessary
- create a way to host this web page outside of the Z-MR for instances where a
  Z-MR is not connected yet.

## zSync

zSync is a connection with an upstream zIQ device. Communication is using the
SIOT data structures that are already in place on the zIQ.

- research connection protocols -- can we use MQTT?
- add MQTT on the Z-MR
- enable MQTT on the Z-IQ (NATS comes with a built-in MQTT server, but we have
  not used it yet).
- create a Z-MR client in Z-IQ -- perhaps use the same Web UI as the Z-MR if
  possible.
- use mDNS to enable Z-MR to discover Z-IQ instances.
- create Z-MR ELM UI to select which Z-IQ instance to connect to
- use MQTT to sync points bi-directionally between Z-MR and Z-IQ.
