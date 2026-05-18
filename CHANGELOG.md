# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

- upgrade to Zephyr v4.4.0 (requires Zephyr SDK 1.0.1)
- migrate point persistence from NVS to ZMS, sized to the storage partition
  so it never writes outside it (NVS could not use STM32H743 128 KiB sectors)
- add `nucleo_h743zi` flash overlay with a 256 KiB storage partition so ZMS
  persistence works on that board
- add `full_name` to custom board definitions (required by Zephyr v4.4.0)
- fix persisted-point default initialization: the first-boot default write
  passed a NULL data pointer, so int/float defaults were never stored and
  the "Error reading <type>" message repeated on every boot
- add `siot-simple` app: minimal serial-shell target exercising lib siot
  and ZMS persistence (boot-count smoke test), no networking
- add `siot_update_west` helper to envsetup.sh

## [0.0.1] - 2025-03-11

- initial development based on Zephyr 3.7+
