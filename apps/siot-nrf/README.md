# SIOT nRF

The SIOT nRF application runs on Nordic nRF cellular modules such as the nRF9151
Feather.

## Building/Flashing

- `siot_build_nrf9151_feather apps/siot-nrf`
- `siot_flash_nrf`
- open serial console: `tio /dev/serial/by-id/usb-Raspberry_Pi_Debug_Probe...`

## Using

- `lte normal`: connect device to cellular network
- `lte offline`: disconnect device from cellular
- `publish`: publish device data to the cloud immediately, rather than waiting
  for the `CONFIG_DEFAULT_DELAY` timer

## Checking the connection

The AT shell sends commands straight to the modem:

- `at AT%XMONITOR`: registration status, operator, band, and cell in one command
- `at AT%CONEVAL`: signal quality while the modem is idle. Reports RSRP, RSRQ,
  and SNR as indices — subtract 140 from RSRP for dBm, and 24 from SNR for dB
- `at AT+CESQ`: signal quality, valid only while a connection is up

`AT+CESQ` and the signal fields of `AT%XMONITOR` read as unknown when the modem
is idle, which is most of the time under PSM. Use `AT%CONEVAL` in that case.

## Publishing

The cloud endpoint defaults to `postman-echo.com`, a request-echo service, so
the publish path can be exercised without standing up a server. It echoes the
request back, so the payload appears in the device log. Set
`CONFIG_CLOUD_HOSTNAME` and `CONFIG_CLOUD_PUBLISH_PATH` to target a real server.

TLS runs inside the modem, so the server's certificate must chain to the CA
provisioned from `src/cloud/isrg-root-x1.pem` — currently ISRG Root X1, which
covers any host with a Let's Encrypt certificate. A server with a certificate
from a different root needs that root swapped in.
