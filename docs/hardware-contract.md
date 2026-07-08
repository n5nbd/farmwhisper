# Hardware contract

This document defines the current high-level expectations for FarmWhisper hardware-facing interfaces and module boundaries.

## Current hardware baseline

- Heltec WiFi LoRa 32 V4 is the current development-base board.
- The built-in display is allowed for development and debugging.
- Production firmware must remain display-optional and must not depend on the display being present.
- The display I2C pins are reserved for display use only and must not be reused for other peripherals.

## Product target

- The first product target is the coop feed sensor.
- The implementation target uses the Heltec V4 development base, one NeoPixel, and a big button.
- The onboard GPIO35 white LED is not a product status indicator and should not be used.

## Base product-node connector concept

FarmWhisper should treat an 8-pin peripheral connector as the preferred base connector for product nodes unless a product has a clear need for more signals.

The selected physical order on Header J3, bottom-up, is:

1. GND
2. 3V3
3. 3V3 / aux 3V3
4. GPIO37 spare / TBD
5. GPIO46 product I2C SCL
6. GPIO45 product I2C SDA
7. GPIO42 big button candidate
8. GPIO41 NeoPixel data candidate

Nearby optional expansion pins for future larger connectors are:

9. GPIO40 optional expansion / GNSS wake-control-labeled pin
10. GPIO39 optional expansion / GNSS TX-labeled pin
11. GPIO38 optional expansion / GNSS RX-labeled pin

This connector is intended to cover common FarmWhisper node needs: I2C sensor bus, power, ground, one status output, one user/service input, and one spare signal.

Ten-pin or larger connectors are allowed only for special node designs with a clear need. The base connector remains 8 pins unless a product has a clear need for more signals.

Display I2C remains separate and reserved for display only. GPIO45/GPIO46 have ESP32-S3 boot/strapping sensitivity, so external circuitry must not strongly drive or load them during boot. GPIO41/GPIO42 may be labeled for GNSS PPS/reset/control on Heltec pinmaps, so they must not be used for FarmWhisper product I/O on any board build where GNSS is populated and expected to function. FarmWhisper base nodes will not use GNSS/GPS by default, so GNSS-associated pins may be claimed for product I/O when GNSS is not populated or used.

## Mechanical and CAD constraints

- CAD source policy: commit .scad only.
- Do not commit .stl, .3mf, .gcode, or slicer or machine output files.
- FW100 CAD tolerance baseline: use fitSlop = 0.30 mm for plastic-to-plastic contact and threadSlop = 0.30 mm for threaded interfaces.

## UI and telemetry convention

- FarmWhisper UI convention: prefix questionable, stale, or unstable readings with ~.
