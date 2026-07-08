# Development environment

This document tracks the local development environment and toolchain expectations for FarmWhisper.

## Current platform baseline

- The current development-base board is the Heltec WiFi LoRa 32 V4.
- The built-in display is acceptable for development and debugging workflows.
- Production firmware must remain display-optional so the product can run without the display.
- The display I2C pins are reserved for display use only and must not be reused for other peripherals.

## Initial checklist

- Git is available for source control.
- A supported text editor or IDE is available for editing documentation, CAD files, and future firmware sources.
- Platform and toolchain choices should be documented here as the project evolves.
- Keep environment-specific settings out of version control.
- This pass is documentation-only; firmware implementation is intentionally deferred.
