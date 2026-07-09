# FarmWhisper

FarmWhisper is a farm monitoring and connectivity project. This repository currently contains the validated component-baseline firmware for the Heltec WiFi LoRa 32 V4 R2/R8 development board, plus FW100 OpenSCAD CAD source.

The current baseline is intentionally small and hardware-focused. It validates the core physical interface before higher-level application work such as WiFi provisioning, captive portal setup, LoRa messaging, persistent configuration, calibration flows, and production node behavior.

Baseline tag:

```text
component-validation-fw100-cad-baseline
```

## Current Status

Validated component baseline:

* Heltec WiFi LoRa 32 V4 R2/R8 development board
* USB CDC serial diagnostics
* GPIO41 NeoPixel status pixel
* GPIO42 active-LOW button input
* Debounced button events:

  * short press
  * long press
  * double press
  * triple press
* Product I2C bus on GPIO45/GPIO46
* VL53L1X ToF sensor validation at I2C address `0x29`
* ToF stability tracking
* Shady/questionable ToF sample marking with leading `~`
* GPIO37/38/39/40 expansion smoke diagnostics as `INPUT_PULLUP`
* FW100 CAD source included as OpenSCAD

Generated print/build artifacts are intentionally excluded from the repository.

## Repository Layout

```text
cad/
  fw100/
    fw100.scad

docs/
  dev-environment.md
  heltec-v4-pinmap.md
  setup-ap.md

firmware/
  include/
    fw_button.h
    fw_config.h
    fw_expansion_gpio.h
    fw_pins.h
    fw_product_i2c.h
    fw_serial_diag.h
    fw_status_pixel.h
    fw_tof.h
    fw_tof_stability.h
    fw_types.h
  src/
    fw_button.cpp
    fw_expansion_gpio.cpp
    fw_product_i2c.cpp
    fw_serial_diag.cpp
    fw_status_pixel.cpp
    fw_tof.cpp
    fw_tof_stability.cpp
    main.cpp
  platformio.ini

hardware/
```

## Firmware Target

The firmware currently targets PlatformIO with the Arduino framework:

```ini
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
```

Validated board:

```text
Heltec WiFi LoRa 32 V4 R2/R8
```

Required PlatformIO libraries:

```text
adafruit/Adafruit NeoPixel
pololu/VL53L1X
```

## Pin Contract

Product interface pins:

| Function                |   GPIO | Notes                    |
| ----------------------- | -----: | ------------------------ |
| Status NeoPixel         | GPIO41 | One-pixel status output  |
| Big button              | GPIO42 | Active LOW               |
| Product I2C SDA         | GPIO45 | Product sensor bus       |
| Product I2C SCL         | GPIO46 | Product sensor bus       |
| Spare / expansion input | GPIO37 | Validated `INPUT_PULLUP` |
| Expansion input         | GPIO38 | Validated `INPUT_PULLUP` |
| Expansion input         | GPIO39 | Validated `INPUT_PULLUP` |
| Expansion input         | GPIO40 | Validated `INPUT_PULLUP` |

The built-in display/OLED I2C pins are reserved for the display only and are not used as the product I2C bus.

## Serial Diagnostics

Default monitor speed:

```text
115200
```

Available serial commands:

| Command    | Action                             |
| ---------- | ---------------------------------- |
| `h` or `?` | Print help                         |
| `s`        | Print status snapshot              |
| `i`        | Rescan product I2C bus             |
| `g`        | Print GPIO37/38/39/40 smoke status |
| `v`        | Toggle verbose ToF sample logging  |
| `r`        | Reset runtime diagnostic counters  |

Example boot output:

```text
===== FarmWhisper component validation baseline =====
[boot] Heltec WiFi LoRa 32 V4 R2/R8
[boot] USB CDC serial enabled
[boot] Product I2C: SDA GPIO45, SCL GPIO46
[boot] Button: GPIO42 active LOW, raw IRQ + debounced app events
[boot] Button events: short press, long press, double press, triple press
[boot] NeoPixel: GPIO41 status model
[boot] GPIO37/38/39/40: spare/expansion GPIO smoke test as INPUT_PULLUP
[boot] Serial diagnostics: h/? help, s status, i i2c scan, g gpio smoke, v tof verbose, r reset counters
[boot] Display/OLED disabled
[boot] LoRa/WiFi/NVS/app calibration not enabled
```

## NeoPixel Status Model

| Status                      | Pixel behavior      |
| --------------------------- | ------------------- |
| Stable valid ToF            | Green               |
| Valid but unstable ToF      | Yellow/orange flash |
| Shady/questionable ToF      | Purple fast flash   |
| ToF timeout or init failure | Red blink           |
| ToF warming                 | Blue pulse          |

Button event overlays:

| Button event | Pixel overlay |
| ------------ | ------------- |
| Short press  | White         |
| Long press   | Cyan          |
| Double press | Blue          |
| Triple press | Magenta       |

## ToF Behavior

The VL53L1X ToF sensor is configured for:

```text
Mode: Long
Timing budget: 50000 us
Continuous period: 100 ms
```

Valid samples update the app-facing last-valid distance and feed the stability window.

Questionable samples are treated as shady, marked with `~` in verbose output, and do not overwrite the last valid distance.

Timeouts are counted separately and do not overwrite the last valid distance.

The current stability window uses five valid samples and reports stable when the sample span is within the configured limit.

## Setup AP and Optional Local PIN

FarmWhisper setup is local-first. WiFi remains off at boot, and the device can
start a temporary setup AP only when requested. The setup AP is reached at:

    http://10.10.10.10/

The local setup PIN is optional:

    No PIN stored -> setup opens directly
    PIN stored    -> setup asks for the PIN first

On the unlocked setup page, saving a 6-digit PIN stores it in ESP32 NVS. Saving
a blank PIN clears it. If the PIN is forgotten, the physical recovery gesture
clears the stored PIN:

    Hold button until LED flashes red five times (~10 seconds).

See [`docs/setup-ap.md`](docs/setup-ap.md) for the setup AP, optional PIN,
recovery, and serial bench workflow.

## Building Firmware

From the repository root:

```bash
cd firmware
pio run
```

Upload:

```bash
pio run -t upload
```

Monitor:

```bash
pio device monitor
```

## CAD

The FW100 CAD source is kept as OpenSCAD source:

```text
cad/fw100/fw100.scad
```

The CAD source uses OpenSCAD/BOSL2-style parametric modeling.

Current fit baseline:

```text
fitSlop = 0.30 mm
threadSlop = 0.30 mm
```

Only source CAD is committed. Generated files are intentionally not committed:

```text
*.stl
*.3mf
*.gcode
```

Builders can export printable files from the OpenSCAD source.

## Artifact Policy

Committed:

```text
.scad source
firmware source
docs
hardware notes/source files
```

Not committed:

```text
.stl
.3mf
.gcode
PlatformIO build output
slicer output
machine-local files
```

## Current Development Direction

The validated baseline establishes the hardware and firmware foundation.

Likely next development slices:

1. WiFi bring-up
2. Captive portal provisioning
3. Persistent device configuration
4. Product/node identity configuration
5. Application-level sensor behavior
6. LoRa/WiFi reporting paths
7. Field-ready packaging and deployment notes

## Baseline Verification

The current baseline was validated with:

* clean PlatformIO build
* boot banner check
* product I2C scan
* VL53L1X detection at `0x29`
* ToF stability/status behavior
* NeoPixel status behavior
* button short/long/double/triple events
* GPIO37/38/39/40 smoke diagnostics
* serial command surface
* FW100 SCAD source committed
