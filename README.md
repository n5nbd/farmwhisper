# FarmWhisper

FarmWhisper is an open-source platform for building distributed agricultural sensors that are easy to install, easy to configure, and designed to operate for long periods with minimal maintenance.

The project combines inexpensive embedded hardware, LoRa radio, Bluetooth Low Energy (BLE), Wi-Fi setup, and modular 3D-printable enclosures into a common firmware platform for a growing family of FarmWhisper devices.

The long-term goal is to support sensors that can be configured from a phone or computer without requiring proprietary software while remaining suitable for remote agricultural deployments.

---

## Project Status

The project has completed its initial hardware-validation phase and is now transitioning into product functionality.

Current development is centered on the Heltec WiFi LoRa 32 V4 platform while the common firmware architecture and hardware interfaces are being established.

The firmware currently supports:

- Wi-Fi setup AP and local setup web application
- Persistent device configuration
- Device alias
- Optional setup PIN with physical recovery
- Firmware-owned LoRa radio profiles
- Firmware-owned transport modes
- LoRa diagnostic transmission
- BLE discovery advertising
- Standard BLE Device Information Service
- FarmWhisper BLE Configuration Service
- Unified alias, radio-profile, and transport-mode configuration through Wi-Fi and BLE
- Modular firmware architecture
- FW100 modular enclosure development

---

## Current Transport Support

| Transport or service | Status |
| --- | --- |
| LoRa diagnostic transport | Supported |
| Bluetooth LE discovery | Supported |
| BLE Device Information Service | Supported |
| BLE Configuration Service | Supported |
| BLE telemetry | Not implemented |

Transport selection is independent of the radio implementation. Firmware stores the selected transport policy, allowing transports to coexist without coupling application configuration to an individual communications module.

---

## Firmware Features

Current firmware provides:

- Persistent configuration stored in ESP32 NVS
- Device alias
- Optional setup PIN protection
- Physical setup-PIN recovery
- Radio-profile selection
- Transport-mode selection
- Shared setup confirmation notices
- Wi-Fi setup application
- Device status endpoint
- Manual LoRa diagnostics
- Continuous listener firmware
- Time-of-Flight (VL53L1X) reference implementation
- Status NeoPixel
- Button gesture interface

BLE currently provides:

- Discovery advertising with the FarmWhisper device identity and alias
- Standard read-only Device Information Service
- FarmWhisper read/write configuration characteristics for:
  - device alias
  - radio profile
  - transport mode

The BLE and Wi-Fi configuration surfaces both use the same firmware-owned configuration setters and validation. BLE telemetry, notifications, streaming, OTA, and general command execution are not implemented.

---

## Firmware Architecture

The firmware is intentionally divided into independent modules.

Major components include:

- `fw_device_config` — persistent alias, selected radio profile, and selected transport mode
- `fw_radio_profile` — firmware-owned radio-profile definitions
- `fw_transport_mode` — firmware-owned transport-policy definitions
- `fw_radio` — LoRa radio implementation
- `fw_ble` — BLE identity, services, advertising, and configuration surface
- `fw_wifi_setup_web` — embedded setup application, setup PIN, routes, and rendering
- `fw_wifi_status` — Wi-Fi radio, setup AP, HTTP-server lifecycle, timeout, scanning, and status
- `fw_tof` — VL53L1X reference implementation
- `fw_button` — button gestures
- `fw_status_pixel` — status LED

The detailed ownership rules and configuration flow are documented in [docs/firmware-architecture.md](docs/firmware-architecture.md).

Keeping these responsibilities separate allows new transports and sensors to be added without restructuring the firmware.

---

## Listener Firmware

The repository also contains a dedicated listener application for validating LoRa communications.

Current listener features include:

- Continuous receive mode
- RSSI reporting
- SNR reporting
- Frequency-error reporting
- Packet counters
- Receive-error counters
- Visual packet indication

The listener is intended as a diagnostic and development tool rather than a production node.

---

## Hardware

Current development hardware:

- Heltec WiFi LoRa 32 V4
- ESP32-S3
- SX1262 LoRa radio
- VL53L1X Time-of-Flight sensor
- WS2812 status LED
- Single-button user interface

The OLED display is optional and primarily used during development. Production devices are intended to operate without a display.

---

## FW100 Enclosure

The FW100 enclosure system is a modular OpenSCAD design intended to support multiple FarmWhisper products while minimizing unique printed parts.

Current enclosure components include:

- Main body
- Conical cap
- Threaded retaining nut
- Load spreader
- Modular internal decks
- Modular retainers

New sensor configurations are created by designing new internal decks rather than redesigning the enclosure.

---

## Repository Layout

```text
cad/
    FW100 OpenSCAD enclosure

docs/
    Project and architecture documentation

firmware/
    PlatformIO firmware

hardware/
    Hardware-related resources
```

---

## Development Philosophy

FarmWhisper development follows a conservative workflow:

1. Make one small change.
2. Build it.
3. Verify it on real hardware when firmware behavior changes.
4. Commit the validated result.
5. Push the checkpoint.

Firmware checkpoints represent code that has been built and tested on physical hardware. Documentation-only checkpoints describe those validated behaviors without changing the firmware image.

---

## Roadmap

Near-term development includes:

- Additional FarmWhisper sensor nodes
- Environmental sensing
- Load-cell support
- Production hardware refinement
- A future FarmWhisper telemetry protocol
- Expanded setup and deployment documentation

FarmWhisper is released as an open-source project. Contributions, experimentation, and constructive feedback are welcome.
