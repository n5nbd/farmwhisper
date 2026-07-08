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

The candidate physical order is:

1. GND
2. 3V3
3. 3V3 / aux 3V3
4. GPIO TBD, likely big button input
5. SDA GPIO45, product I2C bus
6. SCL GPIO46, product I2C bus
7. GPIO TBD, likely NeoPixel data
8. GPIO TBD, spare / interrupt / enable / future use

This connector is intended to cover common FarmWhisper node needs: I2C sensor bus, power, ground, one status output, one user/service input, and one spare signal.

Ten-pin or additional connectors may be used for special nodes, but the 8-pin connector is the preferred base connector unless a product has a clear need for more signals.

Display I2C remains separate and reserved for display only. Final TBD GPIO selection requires physical board and header verification and should avoid bootstrapping, USB, flash/PSRAM, LoRa, display, and other reserved or special pins.

## Mechanical and CAD constraints

- CAD source policy: commit .scad only.
- Do not commit .stl, .3mf, .gcode, or slicer or machine output files.
- FW100 CAD tolerance baseline: use fitSlop = 0.30 mm for plastic-to-plastic contact and threadSlop = 0.30 mm for threaded interfaces.

## UI and telemetry convention

- FarmWhisper UI convention: prefix questionable, stale, or unstable readings with ~.
