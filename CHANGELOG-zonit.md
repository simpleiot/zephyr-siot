# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

- improved workflow with scripts in `envsetup-zonit.sh` (ensures code is
  working, tested, and documented before it is merged)
- added frontend testing (improve frontend code quality)
- added elm-review for frontend (improve frontend code quality)
- code formatting scripts and check
- add display of user switch state
- Fan support:
  - UI to display fan speed/state and settings
  - initial tuning for 35mm fans with 0.25mH/3.25ohm inductor
- Fixed crash when 1-wire sensor is not attached.
- Improve UI rendering on mobile devices
- Added a ATS-MIB which allows to answer getRequest, and a first integration
  test
  - See `apps/z-mr/snmp/snmp_test.sh`
  - See `apps/z-mr/snmp/z-mr.mib.[ch]`
- Enable LLMNR responder in Zephyr. It saves looking up IP-addresses.
- Added static IP, fixed URI loading
- store IPN, ID, HW Version, and MFG Date in EEPROM
