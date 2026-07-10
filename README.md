# FarmWhisper

FarmWhisper is an open-source platform for building distributed agricultural sensors that are easy to install, easy to configure, and designed to operate for long periods with minimal maintenance.

The project combines inexpensive embedded hardware, LoRa radio, Bluetooth Low Energy (BLE), Wi-Fi setup, and modular 3D-printable enclosures into a common firmware platform for a growing family of FarmWhisper devices.

The long-term goal is to support sensors that can be configured from a phone or computer without requiring proprietary software while remaining suitable for remote agricultural deployments.

---

# Project Status

The project has completed its initial hardware validation phase and is now transitioning into product functionality.

Current development is centered on the Heltec WiFi LoRa 32 V4 platform while the common firmware architecture and hardware interfaces are being established.

The firmware currently supports:

* Wi-Fi setup portal
* Persistent device configuration
* Device alias
* Optional setup PIN with physical recovery
* Firmware-owned LoRa radio profiles
* Firmware-owned transport modes
* LoRa diagnostic transmission
* BLE discovery advertising
* Standard BLE Device Information Service
* Modular firmware architecture
* FW100 modular enclosure development

---

# Current Transport Support

| Transport                      | Status    |
| ------------------------------ | --------- |
| LoRa                           | Supported |
| Bluetooth LE Discovery         | Supported |
| BLE Device Information Service | Supported |
| BLE Configuration              | Planned   |
| BLE Telemetry                  | Planned   |

Transport selection is independent of the radio implementation. Firmware stores the selected transport policy, allowing future transports to coexist without coupling application logic to any individual communications module.

---

# Firmware Features

Current firmware provides:

* Persistent configuration stored in NVS
* Device alias
* Setup PIN protection
* Physical PIN recovery
* Radio profile selection
* Transport mode selection
* Shared setup confirmation notices
* Wi-Fi configuration portal
* Device status endpoint
* Manual LoRa diagnostics
* Continuous listener firmware
* Time-of-Flight (VL53L1X) reference implementation
* Status NeoPixel
* Button gesture interface

BLE currently provides standards-based device discovery and identification through the Device Information Service. FarmWhisper-specific BLE configuration and telemetry are planned but intentionally deferred until the core architecture is complete.

---

# Firmware Architecture

The firmware is intentionally divided into independent modules.

Current major components include:

* `fw_device_config` — persistent configuration
* `fw_radio` — LoRa radio implementation
* `fw_radio_profile` — firmware-owned radio profiles
* `fw_transport_mode` — transport policy
* `fw_ble` — Bluetooth Low Energy support
* `fw_wifi_setup_web` — embedded setup application
* `fw_wifi_status` — Wi-Fi and setup state
* `fw_tof` — VL53L1X reference implementation
* `fw_button` — button gestures
* `fw_status_pixel` — status LED

Keeping these responsibilities separate allows new transports and sensors to be added without restructuring the firmware.

---

# Listener Firmware

The repository also contains a dedicated listener application for validating LoRa communications.

Current listener features include:

* Continuous receive mode
* RSSI reporting
* SNR reporting
* Frequency error reporting
* Packet counters
* Receive error counters
* Visual packet indication

The listener is intended as a diagnostic and development tool rather than a production node.

---

# Hardware

Current development hardware:

* Heltec WiFi LoRa 32 V4
* ESP32-S3
* SX1262 LoRa radio
* VL53L1X Time-of-Flight sensor
* WS2812 status LED
* Single-button user interface

The OLED display is considered optional and primarily used during development. Production devices are intended to operate without a display.

---

# FW100 Enclosure

The FW100 enclosure system is a modular OpenSCAD design intended to support multiple FarmWhisper products while minimizing unique printed parts.

Current enclosure components include:

* Main body
* Conical cap
* Threaded retaining nut
* Load spreader
* Modular internal decks
* Modular retainers

New sensor configurations are created by designing new internal decks rather than redesigning the enclosure.

---

# Repository Layout

```text
cad/
    FW100 OpenSCAD enclosure

docs/
    Project documentation

firmware/
    PlatformIO firmware

hardware/
    Hardware-related resources
```

---

# Development Philosophy

FarmWhisper development follows a conservative workflow:

* Make one small change.
* Verify it on real hardware.
* Commit the result.
* Push a validated checkpoint.

Every checkpoint in the repository represents firmware that has been built, tested, and verified on physical hardware.

---

# Roadmap

Near-term development includes:

* BLE configuration service
* Unified configuration through Wi-Fi and BLE
* Additional FarmWhisper sensor nodes
* Environmental sensing
* Load-cell support
* Production hardware refinement
* Expanded documentation

---

FarmWhisper is released as an open-source project. Contributions, experimentation, and constructive feedback are welcome.
