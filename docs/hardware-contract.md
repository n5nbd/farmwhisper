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

## Mechanical and CAD constraints

- CAD source policy: commit .scad only.
- Do not commit .stl, .3mf, .gcode, or slicer or machine output files.
- FW100 CAD tolerance baseline: use fitSlop = 0.30 mm for plastic-to-plastic contact and threadSlop = 0.30 mm for threaded interfaces.

## UI and telemetry convention

- FarmWhisper UI convention: prefix questionable, stale, or unstable readings with ~.
