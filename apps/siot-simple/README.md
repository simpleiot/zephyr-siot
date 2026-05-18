# siot-simple

A minimal Simple IoT application for smoke-testing the shared `lib` and ZMS
persistence on a target. It has **no networking** -- just a serial shell, the
SIOT point/zbus stack, and ZMS-backed point persistence.

See also:

- [Zephyr Simple IoT Library](../../lib/README.md)

## What it exercises

- Serial console + Zephyr shell
- `CONFIG_LIB_SIOT` (point library, zbus `point_chan`/`ticker_chan`, metrics)
- ZMS persistence via `lib/nvs.c`

`bootCount` is persisted. On each boot `lib/nvs.c` reads it, publishes it on
`point_chan`, then increments and writes it back, so it should climb by one
across resets when ZMS is mounting and retaining data -- the quickest way to
confirm persistence. `main.c` adds a zbus listener that logs the value at
`INFO`, so it is visible without enabling lib debug logging.

## Building

```bash
siot_build_nucleo_h743zi apps/siot-simple
# or any other siot_build_<board> target, e.g.
siot_build_esp32_poe apps/siot-simple
```

The bundled `boards/nucleo_h743zi.overlay` enlarges `storage_partition` to
256 KiB (2 x 128 KiB sectors) so ZMS can mount on the STM32H743; see that
file for rationale.

## Verifying persistence

Flash, connect a serial terminal (e.g. `tio`), then reset the board a few
times **without reflashing**. Each boot should log an incrementing:

```
<dbg> nvs_store: Boot count: N
```

If the count resets to 0 every time, ZMS is not retaining data on that
target (check the storage partition size against the flash erase-sector
size).
