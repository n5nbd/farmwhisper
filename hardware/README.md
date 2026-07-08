# Hardware plan

This directory is reserved for FarmWhisper hardware planning and future design notes. No firmware source code or CAD files are added in this pass.

## Current hardware baseline

- Current development base: Heltec WiFi LoRa 32 V4.
- The built-in display may be used for development and debugging only.
- Production firmware must be display-optional.
- Display I2C pins are reserved for display use only.
- The onboard GPIO35 white LED is not a product status indicator and must not be used.

## First product node

- First product node: coop feed sensor.
- Coop feed sensor target hardware:
  - Heltec V4 development board
  - VL53L1X time-of-flight (ToF) sensor
  - single NeoPixel
  - big button

## Base connector concept

FarmWhisper should use an 8-pin peripheral connector as the preferred base connector for product nodes unless a product has a clear need for more signals.

Selected physical order on Header J3, bottom-up:

1. GND
2. 3V3
3. 3V3 / aux 3V3
4. GPIO37 spare / TBD
5. GPIO46 product I2C SCL
6. GPIO45 product I2C SDA
7. GPIO42 big button candidate
8. GPIO41 NeoPixel data candidate

Nearby optional expansion pins for future larger connectors:

9. GPIO40 optional expansion / GNSS wake-control-labeled pin
10. GPIO39 optional expansion / GNSS TX-labeled pin
11. GPIO38 optional expansion / GNSS RX-labeled pin

This connector is intended to cover common FarmWhisper node needs: I2C sensor bus, power, ground, one status output, one user/service input, and one spare signal.

Ten-pin or larger connectors are allowed only for special node designs with a clear need. The base connector remains 8 pins unless a product has a clear need for more signals.

Display I2C remains separate and reserved for display only. GPIO45/GPIO46 have ESP32-S3 boot/strapping sensitivity, so external circuitry must not strongly drive or load them during boot. GPIO41/GPIO42 may be labeled for GNSS PPS/reset/control on Heltec pinmaps, so they must not be used for FarmWhisper product I/O on any board build where GNSS is populated and expected to function. FarmWhisper base nodes will not use GNSS/GPS by default, so GNSS-associated pins may be claimed for product I/O when GNSS is not populated or used.

## Future hardware notes

Reserve room in this directory for notes on:

- power and power delivery
- enclosure and mechanical integration
- sensors and signal conditioning
- connectors and external interfaces
- radio and wireless communication considerations
- field service, debugging, and maintenance

## Notes

- This pass is documentation-only.
- Do not add firmware source code or CAD files here yet.
