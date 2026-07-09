# FarmWhisper serial diagnostics

The component-validation firmware exposes a small serial command interface for hardware bring-up and regression testing.

## Commands

| Command | Purpose |
| --- | --- |
| `h` or `?` | Print help |
| `s` | Print status snapshot |
| `i` | Scan product I2C bus |
| `g` | Print expansion GPIO smoke status |
| `v` | Toggle verbose per-sample ToF logging |
| `a` | Toggle manual WiFi AP smoke test |
| `x` | Print WiFi radio status without scanning |
| `w` | Run one manual WiFi scan, then return WiFi OFF |
| `r` | Reset runtime diagnostic counters |

## WiFi command contract

`x` is read-only. It must not enable STA mode, start AP mode, scan, connect, or modify stored settings.

`w` is temporary. It may enable STA mode only long enough to perform a manual scan. It must force WiFi back OFF before returning.

## Regression rule

Adding a new command must not change behavior of existing commands or boot-time component validation.


## Manual AP smoke test

`a` toggles a temporary open SoftAP named `FarmWhisper-XXXXXX` at `10.10.10.10`.

This command proves the ESP32 can advertise a setup network. It must not start a web server, DNS server, captive portal, credential entry UI, or credential storage.

Press `a` once to start the AP. Press `a` again to stop it.

If `w` is run while AP smoke mode is active, the scan command tears the AP down and returns WiFi to OFF after scanning.


## AP smoke timeout

The AP smoke test times out automatically after 5 minutes.

When the timeout fires, firmware stops the AP and returns WiFi OFF. This prevents `FarmWhisper-XXXXXX` from being left broadcasting indefinitely during bench testing.

## WiFi AP smoke/setup HTTP placeholder

Command `a` toggles the manual `FarmWhisper-XXXXXX` AP. While the AP is active,
the firmware serves a placeholder page at `http://10.10.10.10/`.

The placeholder server exists only for the lifetime of AP smoke/setup mode.
There is no DNS server, captive portal redirect, credential entry, credential
storage, or automatic WiFi behavior in this slice.

## Button setup shortcut

A debounced button triple press starts the WiFi setup AP and placeholder HTTP
server. If setup mode is already active, triple press refreshes the 5-minute
setup timeout.

Serial command `a` remains the manual bench toggle for AP smoke/setup mode.


## WiFi setup status endpoint

While the manual setup AP is active, `http://10.10.10.10/status` returns a
read-only JSON status snapshot for setup/AP HTTP validation. It is available
only while AP setup HTTP mode is running.


## WiFi setup root page

While the manual setup AP is active, `http://10.10.10.10/` serves a simple
server-rendered setup status page. `http://10.10.10.10/status` remains the
read-only JSON status endpoint.


## MAC-derived setup AP name

WiFi setup AP status now reports a device-specific SSID in the form
`FarmWhisper-XXXXXX`, where `XXXXXX` is the last six uppercase hexadecimal
characters of the ESP32 base WiFi MAC address.


## WiFi setup device ID

WiFi setup status now reports a device ID in the form `FWP-XXXXXX`, where
`XXXXXX` is the same MAC suffix used by the setup AP SSID
`FarmWhisper-XXXXXX`.

The device ID appears in serial WiFi status, the setup root page, and the
`/status` JSON endpoint.


## WiFi setup stylesheet route

While setup AP/HTTP mode is active, `http://10.10.10.10/setup.css` serves the
compiled-in stylesheet for the setup root page. This is a presentation-only
route and does not affect WiFi state or diagnostics.


## WiFi setup web module

The WiFi setup web surface is rendered by `fw_wifi_setup_web.{h,cpp}`. Serial
diagnostic behavior is unchanged; the split only moves page/CSS/status route
rendering out of the WiFi state module.
