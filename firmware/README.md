# Firmware plan

This directory is reserved for FarmWhisper firmware planning and future implementation notes. No firmware source code or PlatformIO configuration is added in this pass.

## Intended firmware layout for FarmWhisper nodes

The firmware for FarmWhisper nodes should be structured around a modular architecture that keeps the core application logic independent from hardware-specific details.

### Pin contract note

The firmware now includes a small pin-contract layer in [include/fw_pins.h](include/fw_pins.h) for the Heltec WiFi LoRa 32 V4 development board. The known development pins are declared there, including:

- display SDA on GPIO17
- display SCL on GPIO18
- display reset on GPIO21
- display Vext on GPIO36 (active LOW)
- onboard white LED on GPIO35 (not for product status use)
- development/PRG button input on GPIO0 (active LOW)

The display I2C pins remain reserved for display use only and must not be reused for sensors or other peripherals. Product-specific pins for the NeoPixel and big button are intentionally left as placeholders for future assignment.

### 1. Display-optional architecture

- The firmware should be designed to run with or without the built-in display.
- Display integration must remain optional so production behavior does not depend on a screen being present.
- Core sensing, radio, and input handling should continue to work when the display is absent.

### 2. Board abstraction / pin-contract layer

- A board abstraction layer should define the hardware contract for each supported target board.
- Pin assignments and peripheral capabilities should be centralized so firmware modules remain portable.
- The current development board target is the Heltec WiFi LoRa 32 V4.
- The display I2C pins are reserved for display use only and must not be reused for other peripherals.

### 3. Sensor modules

- Sensor modules should encapsulate the logic for each physical sensing component.
- The first target hardware is the coop feed sensor.
- The initial sensor stack includes a VL53L1X time-of-flight distance sensor.
- Sensor modules should expose clear interfaces for initialization, sampling, calibration, and error state reporting.

### 4. UI and status output modules

- UI and status output modules should handle user-facing feedback and simple diagnostics.
- The first target uses a single NeoPixel for visible status indication.
- A big button should be handled through a dedicated input module that can trigger actions and state changes.
- The onboard GPIO35 white LED is not a product status indicator and should not be used.

### 5. LoRa / radio module

- Radio functionality should be isolated in a dedicated module.
- The radio layer should manage transport setup, packet formatting, and node-to-gateway messaging without embedding radio logic in application code.

### 6. Persistent configuration and NVS

- Persistent settings should be stored in non-volatile storage.
- Configuration values should be versioned and loaded at startup with safe defaults.
- NVS-backed settings are the preferred pattern for node-specific runtime configuration.

### 7. Calibration storage

- Calibration values should be stored separately from general configuration.
- The firmware should support loading, updating, and validating calibration data without coupling it to the main application flow.

## Initial target scope

- First product target: coop feed sensor.
- Hardware baseline: Heltec WiFi LoRa 32 V4 development board.
- Sensor baseline: VL53L1X ToF sensor.
- Output baseline: one NeoPixel.
- Input baseline: big button.

## Notes

- This pass adds the initial firmware skeleton and documentation.
- Firmware source code and PlatformIO files have been added to support the first build cycle, but no hardware-specific features are implemented yet.
- The current PlatformIO target board is assumed to be `esp32-s3-devkitc-1` for Heltec WiFi LoRa 32 V4. Update `firmware/platformio.ini` when the exact Heltec board ID is confirmed.

## Build / upload / monitor

From the `firmware/` directory, use PlatformIO commands once `pio` is installed:

- Build: `pio run`
- Upload: `pio run --target upload`
- Monitor serial output: `pio device monitor`

The firmware boot banner prints over USB serial at 115200 baud.

## Temporary button smoke test

The current skeleton includes a temporary button smoke test on GPIO42 using the candidate big-button pin.

- Wiring: GPIO42 -> button -> GND
- Input mode: INPUT_PULLUP
- Active state: pressed is active LOW

This is a temporary hardware smoke test only; no NeoPixel, ToF, OLED, LoRa, NVS, or calibration behavior is added yet.
