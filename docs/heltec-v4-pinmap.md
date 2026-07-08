# Heltec V4 pin map

This file is an initial reference for Heltec V4-related pin usage and should be updated as hardware decisions are finalized.

## Current contract

- Heltec WiFi LoRa 32 V4 is the current development-base board.
- The built-in display is allowed for development and debugging but production firmware must remain display-optional.
- The display I2C pins are reserved for display use only and must not be reused for other peripherals.
- The first product target is the coop feed sensor using the Heltec V4 dev base, a single NeoPixel, and a big button.
- The onboard GPIO35 white LED is not a product status indicator and should not be used.

## Proposed FarmWhisper 8-pin peripheral header block

This proposal describes a physical header block, not a logical signal-order connector. The candidate pin order below is based on the likely adjacent Heltec V4 header pins and must be verified against the physical board before any final assignment.

Suggested physical order:

1. GND
2. 3V3
3. 3V3 / aux 3V3
4. GPIO TBD, likely big button input
5. SDA GPIO45, product I2C bus
6. SCL GPIO46, product I2C bus
7. GPIO TBD, likely NeoPixel data
8. GPIO TBD, spare / interrupt / enable / future use

This 8-pin connector is intended to cover common FarmWhisper node needs: I2C sensor bus, power, ground, one status output, one user/service input, and one spare signal.

Ten-pin or additional connectors may be used for special nodes, but the 8-pin connector is the preferred base connector unless a product has a clear need for more signals.

Notes:

- Keep GPIO45/GPIO46 as the candidate product I2C bus for VL53L1X and similar sensors.
- Keep display I2C separate and reserved for display only.
- Do not assign button, NeoPixel, or spare GPIO pins yet.
- Final TBD GPIO selection requires physical board and header verification and should avoid bootstrapping, USB, flash/PSRAM, LoRa, display, and other reserved or special pins.

## Notes

- Treat this as a working reference rather than a final authority.
- Confirm pin assignments against the selected board revision and firmware requirements.
- Keep any hardware-specific assumptions documented here for traceability.
