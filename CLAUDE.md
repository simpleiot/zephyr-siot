# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is a Zephyr RTOS workspace for Simple IoT (SIOT) applications, using a [T2 topology](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository) where this repository is the manifest repository. The project contains firmware applications for various microcontrollers (ESP32, STM32, nRF) that implement IoT functionality with a point-based data architecture.

## Initial Setup

After cloning, run these commands:
```bash
cd siot
. envsetup.sh  # Note the leading dot
siot_setup
```

The `siot_setup` function performs:
- `west update` - updates all modules
- `west packages pip --install` - installs Python dependencies
- `west blobs fetch hal_espressif` - fetches ESP32 HAL blobs
- `npm install` - installs Node.js dependencies for tooling

## Building Applications

Build commands follow the pattern `siot_build_<board> <app_path>`:

```bash
# ESP32 targets
siot_build_esp32_poe apps/siot-net
siot_build_esp32_poe_wrover apps/siot-net
siot_build_esp32_wroom apps/siot-serial
siot_build_esp32_wrover apps/siot-serial
siot_build_esp32c3_devkitc apps/siot-serial
siot_build_esp32c3_devkitm apps/siot-serial
siot_build_esp32_ethernet_kit apps/siot-net

# STM32 targets
siot_build_nucleo_h743zi apps/siot-net
siot_build_nucleo_l452re apps/siot-serial
siot_build_nucleo_l432kc apps/siot-serial

# Nordic nRF targets
siot_build_nrf9151_feather apps/siot-nrf

# Raspberry Pi Pico
siot_build_rpi_pico2 apps/siot-serial

# Native simulation (for testing)
siot_build_native_sim tests
```

All builds create output in the `build/` directory at the repository root.

## Flashing Firmware

Flashing commands vary by target:

```bash
# For STM32 and nRF targets with J-Link
siot_flash

# For ESP32 targets (requires serial port)
siot_flash_esp /dev/ttyUSB0

# For Olimex ESP32-POE boards (using known device path)
siot_flash_olimex_esp32_poe

# For nRF with pyOCD
siot_flash_nrf

# For RP2040/RP2350 via UF2 bootloader
siot_flash_uf2
```

## Testing

Run all library tests on native platform:
```bash
siot_test_native
```

This builds and runs tests in `tests/` using the `native_sim` board.

## Code Formatting

The project uses clang-format for C code and prettier for markdown:

```bash
siot_format              # Format all code
siot_format_check        # Check formatting without modifying files
```

## Architecture

### Point-Based Data Model

The core abstraction is a **point** (defined in `include/point.h`), a 73-byte struct representing configuration and sensor data:

| Field | Type | Size | Description |
|-------|------|------|-------------|
| `time` | `uint64_t` | 8 bytes | Nanoseconds since Unix epoch |
| `type` | `char[]` | 24 bytes | Point type (e.g., "temp", "bootCount") |
| `key` | `char[]` | 20 bytes | Index or key for arrays/maps |
| `data_type` | `uint8_t` | 1 byte | Encoding: float, int, or string |
| `data` | `uint8_t[]` | 20 bytes | Actual data payload |

### Thread Architecture

Applications use [Zephyr zbus](https://docs.zephyrproject.org/latest/services/zbus/index.html) for inter-thread communication. Multiple threads publish and subscribe to point channels:

- 1-wire thread: reads sensors, publishes temperature points
- Web thread: serves HTTP API, publishes/subscribes to points for UI
- Fan control thread: subscribes to temperature points, controls hardware
- NVS thread: listens for specific point types and persists them to flash

This architecture decouples threads - a sensor thread doesn't need to know about web or control threads.

### Persistent Storage

The [NVS subsystem](https://docs.zephyrproject.org/latest/services/storage/nvs/nvs.html) stores settings in flash. To persist a point type:

1. Add it to the `nvs_pts` table in your application's `main.c`
2. The NVS thread automatically saves these points when they appear on the zbus channel
3. On boot, saved points are read from flash and republished to configure the system

See `apps/siot-net/src/main.c` for an example.

### Ticker Channel

A message is sent to the zbus `ticker_chan` every 500ms for timing and periodic tasks.

## Applications

| Application | Description | Key Features |
|------------|-------------|--------------|
| **siot-net** | Network-connected device with web UI | HTTP server, Elm SPA frontend, Ethernet/WiFi |
| **siot-serial** | Serial-connected device | UART communication, shell interface |
| **siot-nrf** | Nordic nRF9151 cellular application | LTE-M/NB-IoT connectivity |
| **siot-nrf-simple** | Simplified nRF9151 example | Minimal cellular example |
| **siggen** | Signal generator | DAC output for waveform generation |
| **siot-can-node** | CAN bus node | Controller Area Network communication |

All applications have the Zephyr shell enabled - connect via serial (recommended: [tio](https://github.com/tio/tio)).

## Web Frontend (siot-net)

The web UI is an Elm single-page application in `apps/siot-net/frontend/`.

### Development Workflow

```bash
# Start development server with live reload
siot_net_frontend_watch

# Build production frontend (generates compressed assets)
siot_net_frontend_build

# Format Elm code
siot_net_frontend_format
siot_net_frontend_format_check

# Run tests
siot_net_frontend_test  # Runs elm-test and elm-review
```

**Note:** Update the `proxy` field in `apps/siot-net/frontend/elm-land.json` with your target device's IP address for development.

The production build:
1. Compiles Elm to JavaScript
2. Moves assets to the root of `dist/`
3. Renames files for consistent inclusion
4. Updates `index.html` references

These assets are then compressed and embedded into the firmware binary via CMake's `generate_inc_file_for_target` (see `apps/siot-net/CMakeLists.txt`).

## Memory Management

Monitor stack usage with:
```bash
siot_ram_report
siot_rom_report
```

Keep thread stacks under 70% usage (check with `kernel thread stacks` shell command).

For debugging memory issues, enable in `prj.conf`:
```
CONFIG_ASSERT=y
CONFIG_ASSERT_VERBOSE=y
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_STACK_CANARIES=y
CONFIG_STACK_SENTINEL=y
```

## Debugging

View generated configuration:
```bash
siot_peek_generated_config  # Opens autoconf.h
siot_peek_generated_dts     # Opens device tree
```

Interactive configuration:
```bash
siot_menuconfig
```

Crash debugging with [west-debug-tools](https://github.com/hasheddan/west-debug-tools):
```bash
west dbt addr2src 0x400fc9f1  # Converts address to source location
```

## Custom Boards

Custom board definitions are in `boards/`:
- `esp32_poe` - Olimex ESP32-POE
- `esp32_poe_wrover` - Olimex ESP32-POE with WROVER module (more RAM)
- `fysetc_ucan` - CAN bus board

Board-specific configurations are in `apps/<app>/boards/`.

## Library Code

Common code shared across applications is in `lib/`:

| File | Purpose |
|------|---------|
| `point.c` | Point data structure and JSON encoding/decoding |
| `nvs.c` | Non-volatile storage management for points |
| `html.c` | HTML generation utilities |
| `metrics.c` | System metrics collection |
| `zbus.c` | Zephyr bus initialization and utilities |
| `siot-string.c` | String manipulation helpers |

Headers are in `include/`.

## West Manifest

The `west.yml` manifest defines:
- Zephyr version (currently v4.3.0)
- HAL modules (ESP32, STM32, Raspberry Pi Pico)
- External modules:
  - `zephyr-snmp` - SNMP support
  - `module-sample` - Example module
  - `west-debug-tools` - Debugging utilities

## Versioning and Tagging

Extract version from an application's VERSION file:
```bash
siot_extract_version apps/siot-net/VERSION
```

Tag a release (must be on main branch with no uncommitted changes):
```bash
siot_tag_version siot-net  # Creates tag like siot-net_v1.0.0
```

## Key Configuration Patterns

### Network Configuration

Network apps typically need (see `apps/siot-net/prj.conf`):
```
CONFIG_NETWORKING=y
CONFIG_NET_IPV4=y
CONFIG_NET_DHCPV4=y
CONFIG_NET_TCP=y
CONFIG_HTTP_SERVER=y
CONFIG_NET_SOCKETS=y
```

### Shell Configuration

All apps enable shell for debugging:
```
CONFIG_SHELL=y
CONFIG_I2C_SHELL=y  # For I2C debugging
CONFIG_NET_SHELL=y  # For network debugging
```

## File Organization

```
siot/
├── apps/              # Application firmware
│   ├── siot-net/      # Network app with web UI
│   ├── siot-serial/   # Serial communication app
│   ├── siot-nrf/      # Nordic cellular app
│   └── ...
├── boards/            # Custom board definitions
├── include/           # Public library headers
├── lib/               # Shared library code
├── tests/             # Unit tests
├── envsetup.sh        # Build environment setup script
├── west.yml           # West manifest (dependency management)
└── CMakeLists.txt     # Top-level build configuration
```

## Important Points

- Always source `envsetup.sh` at the start of each session: `. envsetup.sh`
- The build system uses West (Zephyr's meta-tool)
- Board-specific files override application defaults
- Custom boards require `-- -DBOARD_ROOT="$(pwd)"` in build commands
- Frontend assets are compressed and embedded in firmware at build time
- Point types should align with the [SIOT schema](https://github.com/simpleiot/simpleiot/blob/master/data/schema.go)
