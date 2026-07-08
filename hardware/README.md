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
