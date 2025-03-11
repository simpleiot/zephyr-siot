# Simple IoT Zephyr Networking Example

See also:

- [Zephyr Simple IoT Library](../../lib/README.md)

This application is an example of how to use the SIOT architecture for a network
connected MCU.

![ui](assets/image-20250130111410493.png)

## Web UI Frontend

The web UI for the project is a single-page application (SPA) written in Elm
that reads and writes JSON data to an HTTP API implemented in the Zephyr `web.c`
module.

[elm-land](https://elm.land/) comes with a development server that runs on your
workstation and updates the web page whenever it changes without losing state.
This is useful because you can do all of the web frontend develoment without
reflashing the Z-MR.

To run the development server: `siot_net_frontend_watch`

You will need to update the proxy field in
`apps/siot-net/frontend/elm-land.json` to match the IP address of your Z-MR
target system. (long term we'd like this to be an environment variable, but that
is not working yet)
