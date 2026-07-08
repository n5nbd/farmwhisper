# Heltec V4 pin map

This file is an initial reference for Heltec V4-related pin usage and should be updated as hardware decisions are finalized.

## Current contract

- Heltec WiFi LoRa 32 V4 is the current development-base board.
- The built-in display is allowed for development and debugging but production firmware must remain display-optional.
- The display I2C pins are reserved for display use only and must not be reused for other peripherals.
- The first product target is the coop feed sensor using the Heltec V4 dev base, a single NeoPixel, and a big button.
- The onboard GPIO35 white LED is not a product status indicator and should not be used.

## Proposed FarmWhisper 8-pin peripheral header block

This proposal describes a physical header block, not a logical signal-order connector. The selected 8-pin base peripheral connector uses the same Header J3 bottom-up GPIO order on both observed Heltec WiFi LoRa 32 V4 R2 and R8 pinouts.

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

This 8-pin connector is intended to cover common FarmWhisper node needs: I2C sensor bus, power, ground, one status output, one user/service input, and one spare signal.

Ten-pin or larger connectors are allowed only for special node designs with a clear need. The base connector remains 8 pins unless a product has a clear need for more signals.

Notes:

- Keep GPIO45/GPIO46 as the product I2C bus for VL53L1X and similar sensors.
- Keep display I2C separate and reserved for display only.
- Do not assign the TBD GPIO pins yet.
- GPIO45/GPIO46 have ESP32-S3 boot/strapping sensitivity, so external circuitry must not strongly drive or load them during boot. They remain the product I2C bus because they were verified working on the Heltec V4 ToF mule.
- GPIO41/GPIO42 may be labeled for GNSS PPS/reset/control on Heltec pinmaps, so they must not be used for FarmWhisper product I/O on any board build where GNSS is populated and expected to function.
- FarmWhisper base nodes will not use GNSS/GPS by default, so GNSS-associated pins may be claimed for product I/O when GNSS is not populated or used.
- Final GPIO selection requires physical board and header verification and should avoid bootstrapping, USB, flash/PSRAM, LoRa, display, and other reserved or special pins.

## Notes

- Treat this as a working reference rather than a final authority.
- Confirm pin assignments against the selected board revision and firmware requirements.
- Keep any hardware-specific assumptions documented here for traceability.
