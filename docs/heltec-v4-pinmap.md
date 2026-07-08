# Heltec V4 pin map

This file is an initial reference for Heltec V4-related pin usage and should be updated as hardware decisions are finalized.

## Current contract

- Heltec WiFi LoRa 32 V4 is the current development-base board.
- The built-in display is allowed for development and debugging but production firmware must remain display-optional.
- The display I2C pins are reserved for display use only and must not be reused for other peripherals.
- The first product target is the coop feed sensor using the Heltec V4 dev base, a single NeoPixel, and a big button.
- The onboard GPIO35 white LED is not a product status indicator and should not be used.

## Notes

- Treat this as a working reference rather than a final authority.
- Confirm pin assignments against the selected board revision and firmware requirements.
- Keep any hardware-specific assumptions documented here for traceability.
