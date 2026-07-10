# FarmWhisper Firmware Architecture

This document describes the current firmware ownership boundaries and configuration flow for the validated Heltec WiFi LoRa 32 V4 development baseline.

It documents the architecture as implemented at the BLE Configuration Service checkpoint. It is not a list of planned features.

## Design Rules

The firmware follows these rules:

1. `main.cpp` remains orchestration-only.
2. Hardware behavior belongs in dedicated modules.
3. User-selectable options are defined by firmware-owned models.
4. Configuration surfaces do not duplicate validation or persistence logic.
5. Transport policy remains separate from transport implementations.
6. Persistent selection and applied runtime state are tracked separately when needed.
7. Wi-Fi remains off unless a deliberate diagnostic or setup action enables it.
8. A functional firmware slice is built, uploaded, hardware-tested, committed, and pushed before the next functional slice.

## Top-Level Configuration Flow

```text
                      FarmWhisper configuration surfaces

             Wi-Fi setup application         BLE Configuration Service
             fw_wifi_setup_web                fw_ble
                       |                          |
                       | trim / normalize         | trim / normalize
                       +------------+-------------+
                                    |
                                    v
                         fw_device_config setters
                         and shared validation
                                    |
                    +---------------+----------------+
                    |               |                |
                    v               v                v
               device alias    radio profile    transport mode
                    |               |                |
                    |               |                |
                    v               v                v
             BLE identity       fw_radio      fw_ble / main policy
             setup status       selected vs.  LoRa and BLE enablement
                                applied state

Setup PIN path:

    Wi-Fi setup application
             |
             v
    fw_wifi_setup_web
    local authorization and NVS ownership

The setup PIN is intentionally not part of the BLE Configuration Service.
```

## Persistent Configuration Ownership

FarmWhisper currently has four persistent configuration fields, but they are not all owned by the same module.

| Field | Owner | Storage | Configuration surfaces |
| --- | --- | --- | --- |
| Device alias | `fw_device_config` | NVS namespace `fwdevcfg`, key `alias` | Wi-Fi and BLE |
| Radio profile selection | `fw_device_config` | NVS namespace `fwdevcfg`, key `radioProfile` | Wi-Fi and BLE |
| Transport mode selection | `fw_device_config` | NVS namespace `fwdevcfg`, key `transportMode` | Wi-Fi and BLE |
| Optional setup PIN | `fw_wifi_setup_web` | NVS namespace `fw_setup`, key `pin` | Wi-Fi setup only |

`fw_device_config` is the single authority for validating, saving, loading, and exposing the device alias, radio-profile selection, and transport-mode selection.

`fw_wifi_setup_web` owns the setup PIN because the PIN protects the local setup session. It also owns the temporary unlocked state and the physical-recovery storage operation exposed through `fw_wifi_status`.

## Firmware-Owned Models

### Radio Profiles

`fw_radio_profile` owns the available named LoRa profiles and their internal radio parameters.

Current keys:

- `us-default`
- `us-long-range`

User-facing configuration surfaces select a profile by key or display name. They do not expose raw frequency, bandwidth, spreading factor, coding rate, preamble, or power as editable fields.

`fw_radio` reads the selected profile from `fw_device_config`.

The radio tracks both:

- selected profile
- applied profile

Before transmission, `fw_radio` initializes or reinitializes the SX1262 when the selected profile is not the profile currently applied to the radio.

### Transport Modes

`fw_transport_mode` owns the available transport policies.

Current keys:

- `lora`
- `bluetooth-le`
- `lora-bluetooth-le`

The model also answers policy questions:

- whether a mode uses LoRa
- whether a mode uses Bluetooth LE

Transport policy does not belong in `fw_radio` or `fw_ble`. Those modules consume the policy.

## Configuration Surfaces

### Wi-Fi Setup Application

`fw_wifi_status` owns:

- Wi-Fi radio state
- one-shot Wi-Fi scanning
- setup AP start and stop
- the `10.10.10.10` address contract
- MAC-derived device ID and setup SSID
- AP timeout
- `WebServer` lifecycle
- periodic `handleClient()` servicing
- the read-only status snapshot supplied to the web renderer

`fw_wifi_setup_web` owns:

- route registration
- HTML and CSS rendering
- `/status` JSON rendering
- setup PIN storage and validation
- setup-session lock and unlock state
- shared confirmation and error notices
- alias, radio-profile, and transport-mode form handlers

For alias, radio profile, and transport mode, the web handlers trim the submitted text and call `fw_device_config`. They do not validate profile or mode keys independently.

The current setup application is local and manually activated. It does not yet provide DNS captive-portal redirection or Wi-Fi network credential provisioning.

### BLE

`fw_ble` owns:

- BLE stack initialization
- MAC-derived FarmWhisper device identity
- advertised local name
- transport-controlled advertising
- advertising restart after disconnect
- standard Device Information Service
- FarmWhisper Configuration Service
- synchronization of characteristic values with current firmware state

The standard Device Information Service uses UUID `0x180A` and exposes read-only manufacturer, model, serial number, firmware revision, and hardware revision characteristics.

The FarmWhisper Configuration Service uses:

```text
Service:
3f4b0001-7d5a-4b22-9e2f-5c7a9f6d1000

Device alias:
3f4b0002-7d5a-4b22-9e2f-5c7a9f6d1000

Radio profile:
3f4b0003-7d5a-4b22-9e2f-5c7a9f6d1000

Transport mode:
3f4b0004-7d5a-4b22-9e2f-5c7a9f6d1000
```

All three FarmWhisper characteristics are read/write text values.

They do not provide:

- notifications
- indications
- telemetry
- streaming
- OTA
- general command execution

BLE write callbacks trim the incoming value and call the same `fw_device_config` setters used by the Wi-Fi application.

A blank alias clears the stored alias and restores the firmware default. Unknown radio-profile and transport-mode keys are rejected. After every write, the characteristics are refreshed from firmware state so the readback value remains authoritative.

## Runtime Consumers

### BLE Reconciliation

`FWBLE::service()` runs from the main loop.

It reconciles BLE state against:

- selected transport mode
- current device alias
- client connection state
- pending advertising restart delay

Changing the alias causes the advertised name to be rebuilt. Changing transport mode can start or stop advertising without a reboot.

### LoRa Policy and Radio State

The main button diagnostic path asks `fw_transport_mode` whether the selected mode uses LoRa before calling the radio transmitter.

`fw_radio` owns SX1262 initialization and transmission. It does not own transport selection.

The radio profile can be changed while the radio is already initialized. The next transmission compares selected and applied profiles and reinitializes the radio when they differ.

### Status Surfaces

The serial status path and `/status` endpoint read current values from the owning modules. They do not maintain separate configuration copies.

## Main Loop Responsibilities

`main.cpp` coordinates modules but does not implement their internal behavior.

Current high-level sequence:

```text
setup()
    serial diagnostics
    Wi-Fi baseline initialization
    BLE policy reconciliation
    status pixel
    button
    expansion GPIO
    product I2C
    ToF sensor

loop()
    Wi-Fi/AP/HTTP service
    BLE reconciliation
    serial commands
    button update and gestures
    physical setup-PIN recovery
    transport-aware LoRa diagnostic
    setup AP activation
    ToF polling
    status pixel update
    heartbeat
```

## Hardware and Diagnostic Modules

| Module | Ownership |
| --- | --- |
| `fw_pins` | Validated board and product pin contract |
| `fw_types` | Small shared data types |
| `fw_config` | Shared timing and diagnostic constants |
| `fw_status_pixel` | GPIO41 NeoPixel and status output |
| `fw_button` | GPIO42 active-LOW button and gestures |
| `fw_product_i2c` | Product I2C initialization and scanning |
| `fw_expansion_gpio` | GPIO37/38/39/40 smoke diagnostics |
| `fw_tof_stability` | Stable, shady, timeout, and last-valid ToF classification |
| `fw_tof` | VL53L1X initialization and polling |
| `fw_serial_diag` | Serial command and status interface |
| `fw_wifi_status` | Wi-Fi, AP, server lifecycle, timeout, scanning, and status |
| `fw_wifi_setup_web` | Setup routes, rendering, PIN, and configuration handlers |
| `fw_device_config` | Alias, radio-profile selection, and transport-mode persistence |
| `fw_radio_profile` | Firmware-owned LoRa profiles |
| `fw_transport_mode` | Firmware-owned transport policies |
| `fw_radio` | SX1262 implementation and applied-profile state |
| `fw_ble` | BLE identity, services, advertising, and configuration |

## Extension Rules

Future work should preserve these boundaries:

- Add new radio profiles in `fw_radio_profile`, not in web or BLE handlers.
- Add new transport modes in `fw_transport_mode`, not in `fw_radio`.
- Add persistent product settings through an owning configuration module with shared setters.
- Make every configuration surface call the owning module instead of duplicating validation.
- Keep telemetry separate from configuration.
- Keep setup authorization separate from general device configuration.
- Keep `main.cpp` limited to module orchestration and cross-module policy decisions.
- Do not expose firmware-owned raw radio parameters as user-editable controls.
- Do not make product behavior depend on an OLED display.

## Validated Checkpoint

At this checkpoint, physical hardware testing has confirmed:

- Wi-Fi setup changes alias, radio profile, and transport mode
- BLE changes the same three fields
- alias changes update BLE identity and persist across restart
- radio-profile changes persist and invalid values are rejected
- transport-mode changes immediately control BLE availability
- BLE can be disabled by selecting LoRa-only mode and restored through the setup/configuration path
- the configuration service contains no telemetry, notifications, OTA, or general commands
