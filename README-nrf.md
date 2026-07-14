# nRF9151 memory notes

Memory layout and headroom for the `siot-nrf` application on the Circuit Dojo
nRF9151 Feather.

All numbers below come from the current build:

```bash
. envsetup.sh
siot_build_nrf9151_feather apps/siot-nrf
```

This is a sysbuild with three images: MCUboot, TF-M (secure), and the Zephyr
application (non-secure).

## Hardware budget

The nRF9151 SiP provides:

| Resource       | Size   | Notes                             |
| -------------- | ------ | --------------------------------- |
| Internal flash | 1 MB   | `0x0` – `0x100000`                |
| SRAM           | 256 KB | `0x20000000` – `0x20040000`       |
| External flash | 16 MB  | W25Q128JV on SPI3, board-provided |

The modem firmware lives in its own dedicated memory inside the SiP and does not
consume any of the above.

## Flash layout

Partition Manager divides the 1 MB of internal flash as follows. The `Used`
column is the size of the image actually placed in the partition.

| Partition           | Range                  | Size             | Used                     |
| ------------------- | ---------------------- | ---------------- | ------------------------ |
| `mcuboot`           | `0x00000` – `0x0c000`  | 48 KB (49,152)   | 47,832 B — **97%**       |
| `EMPTY_0`           | `0x0c000` – `0x10000`  | 16 KB (16,384)   | alignment padding        |
| `mcuboot_pad`       | `0x10000` – `0x10200`  | 512 B            | MCUboot image header     |
| `tfm`               | `0x10200` – `0x18000`  | 31.5 KB (32,256) | 31,704 B — **98%**       |
| `app`               | `0x18000` – `0x88000`  | 448 KB (458,752) | 133,024 B — **29%**      |
| `mcuboot_secondary` | `0x88000` – `0x100000` | 480 KB (491,520) | empty — DFU staging slot |

`mcuboot_primary` (`0x10000` – `0x88000`, 480 KB) is the span of `mcuboot_pad` +
`tfm` + `app`. That combined image is what MCUboot verifies and what gets
signed:

- Signed image: 165,240 B of the 480 KB slot — **34% used**.

The `tfm` partition is sized to fit its image, so the 98% figure is expected
rather than a warning sign. The MCUboot partition, on the other hand, is a fixed
48 KB and is genuinely close to full.

### External flash

The board carries a 16 MB W25Q128JV. The device tree assigns the whole chip to
`lfs_partition`. Partition Manager currently models a 2 MB `external_flash`
region and places nothing in it, so the external flash is entirely available
today. Worth knowing: the PM region size and the device tree chip size disagree,
which matters if we ever ask Partition Manager to place a partition there.

The external flash cannot be executed from. Execute-in-place on Nordic parts
requires a QSPI peripheral that memory-maps the flash into the address space,
and the nRF91 series does not have one — `nrf91_peripherals.dtsi` declares no
QSPI node, and the W25Q128JV is attached over SPI3. The chip is therefore a block
device: its contents must be read into RAM before the CPU can use them.

That makes it useful for DFU staging and for data storage (LittleFS, logs,
buffered telemetry, credentials), but it does not relieve the code size limit.
All executable code has to fit in the 1 MB of internal flash, so the ~900 KB
figure below is a genuine upper bound on application size no matter how the
external flash is used.

## RAM layout

The 256 KB of SRAM is divided between the secure image, the modem library's
shared memory, and the non-secure application:

| Region               | Range                       | Size               |
| -------------------- | --------------------------- | ------------------ |
| `tfm_sram` (secure)  | `0x20000000` – `0x20008000` | 32 KB (32,768)     |
| `nrf_modem_lib_ctrl` | `0x20008000` – `0x200084e8` | 1,256 B            |
| `nrf_modem_lib_tx`   | `0x200084e8` – `0x2000a568` | 8,320 B            |
| `nrf_modem_lib_rx`   | `0x2000a568` – `0x2000c568` | 8,192 B            |
| `sram_primary` (app) | `0x2000c568` – `0x20040000` | 206.6 KB (211,608) |

The three `nrf_modem_lib_*` regions are the shared-memory interface to the modem
core and total 17,768 B (17.4 KB). They are reserved regardless of traffic.

MCUboot links its RAM at `0x20000000` with a 32 KB window — the same memory TF-M
later owns. MCUboot has exited by the time TF-M runs, so this is reuse rather
than an additional cost.

### Application RAM usage

The non-secure application's static footprint:

| Section          | Size                                      |
| ---------------- | ----------------------------------------- |
| `data`           | 21,688 B                                  |
| `bss`            | 43,509 B                                  |
| **Total static** | **65,197 B — 31% of the 206.6 KB region** |

Thread stacks and the 8 KB system heap (`CONFIG_HEAP_MEM_POOL_SIZE=8192`) are
allocated in `bss` and are already counted above.

## Available headroom

This is the practical answer to "how much room do we have to build on":

| Resource                  | Total    | Used    | Free       |
| ------------------------- | -------- | ------- | ---------- |
| Application flash (`app`) | 448 KB   | 130 KB  | **318 KB** |
| Application RAM           | 206.6 KB | 63.7 KB | **143 KB** |
| External flash            | 16 MB    | 0       | **16 MB**  |

### The flash ceiling

The 448 KB `app` partition cannot simply be grown. MCUboot's swap-based DFU
requires the primary and secondary slots to be the same size, and the current
layout already spends the full 1 MB:

```
48 KB (mcuboot) + 16 KB (padding) + 480 KB (primary) + 480 KB (secondary) = 1 MB
```

So 318 KB of application growth is the hard limit in this configuration. If we
need more, the option with the most upside is moving the secondary slot to the
external flash chip (`CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY=y`). That frees
480 KB of internal flash and would let the application partition roughly double,
to somewhere near 900 KB, at the cost of a slower DFU (the image is copied from
SPI flash) and a dependency on the external flash part being populated.

## Where the flash goes

Breakdown of the 133 KB application image:

| Component     | Size     | Share | Notes                                        |
| ------------- | -------- | ----- | -------------------------------------------- |
| `ZEPHYR_BASE` | 59,858 B | 45%   | Zephyr itself                                |
| `(no paths)`  | 35,968 B | 27%   | Newlib, compiler runtime, prebuilt libraries |
| `WORKSPACE`   | 23,565 B | 18%   | NCS and SIOT code                            |
| `(hidden)`    | 12,981 B | 10%   | Symbols the report cannot attribute          |

Within Zephyr, the largest consumers are:

| Subsystem        | Size     | Share |
| ---------------- | -------- | ----- |
| `subsys/shell`   | 16,046 B | 12%   |
| `lib`            | 12,058 B | 9%    |
| `kernel`         | 10,580 B | 8%    |
| `drivers`        | 7,266 B  | 6%    |
| `subsys/net`     | 4,016 B  | 3%    |
| `subsys/logging` | 3,900 B  | 3%    |

Within the workspace:

| Component   | Size     | Share |
| ----------- | -------- | ----- |
| `nrf` (NCS) | 15,778 B | 12%   |
| `modules`   | 5,008 B  | 4%    |
| `siot`      | 2,631 B  | 2%    |

Two observations worth carrying forward:

- The shell is the single largest identifiable subsystem at 16 KB. It is
  valuable during development, and disabling `CONFIG_SHELL` is the most direct
  saving available for a production image.
- SIOT's own code is 2.6 KB, about 2% of the image. Nearly all of the current
  footprint is platform, not application, so there is real room to grow the
  application before flash becomes the constraint.

## Reproducing these numbers

Sysbuild puts each image in its own subdirectory, so the reports need an
explicit build directory:

```bash
# Application (non-secure) ROM and RAM reports
west build -d build/siot-nrf -t rom_report
west build -d build/siot-nrf -t ram_report

# Raw section sizes for each image
arm-zephyr-eabi-size build/siot-nrf/zephyr/zephyr.elf   # application
arm-zephyr-eabi-size build/siot-nrf/tfm/bin/tfm_s.elf   # TF-M secure
arm-zephyr-eabi-size build/mcuboot/zephyr/zephyr.elf    # MCUboot
```

The authoritative partition map is generated at build time:

```bash
cat build/partitions.yml   # every partition, with address and size
cat build/regions.yml      # the flash and RAM regions they are placed in
```

At runtime, the Zephyr shell reports live stack usage:

```
kernel thread stacks
```

Keep thread stacks under 70% usage.
