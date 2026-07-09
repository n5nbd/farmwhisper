# FarmWhisper firmware architecture

This document describes the component-validation firmware structure after the validated Heltec V4/FW100 baseline refactor.

The current firmware is intentionally modular. `main.cpp` should remain orchestration-only: setup modules, call periodic service functions, and avoid owning hardware behavior directly.

## Validated hardware baseline

The current hardware regression baseline is Heltec WiFi LoRa 32 V4 R2/R8 with:

- GPIO41 NeoPixel status pixel
- GPIO42 active-LOW button
- GPIO45/GPIO46 product I2C bus
- VL53L1X ToF sensor at I2C address `0x29`
- GPIO37/38/39/40 expansion smoke diagnostics as `INPUT_PULLUP`

## Module ownership

| Module | Ownership |
| --- | --- |
| `fw_pins` | Validated pin contract |
| `fw_types` | Small shared data types |
| `fw_config` | Shared timing/config constants |
| `fw_status_pixel` | GPIO41 NeoPixel/status output |
| `fw_button` | GPIO42 active-LOW button events |
| `fw_product_i2c` | Product I2C bus initialization/scanning |
| `fw_expansion_gpio` | GPIO37/38/39/40 smoke diagnostics |
| `fw_tof_stability` | Stable/shady/timeout/last-valid ToF classification |
| `fw_tof` | VL53L1X bring-up and polling |
| `fw_serial_diag` | Serial command interface |
| `fw_wifi_status` | Manual WiFi status/scan diagnostics |

## Comment standard

Comments should preserve design intent for future maintenance.

Use comments for:

- hardware contracts
- validated behavior
- non-obvious timing or state decisions
- safety constraints
- future extension boundaries

Avoid comments that only restate obvious syntax.

## Regression rule

Every functional slice should be built, uploaded, hardware-tested, and committed before the next slice.

A change is not considered safe until the existing component-validation behavior still passes.
