# FarmWhisper WiFi bring-up

This document tracks WiFi bring-up from the component-validation firmware baseline.

The current WiFi work is diagnostic-only. The validated hardware behavior must remain unchanged unless a manual serial command is entered.

## Current behavior

WiFi is OFF at boot.

The firmware does not:

- connect to an access point
- start an access point
- start a captive portal
- store credentials
- write WiFi settings to flash
- scan automatically at boot

## Serial diagnostics

### `a` — WiFi AP smoke test

Toggles a temporary open SoftAP named `FarmWhisper-XXXXXX` at `10.10.10.10`.

This is only a radio smoke test. It does not start a web server, DNS server, captive portal, credential UI, or credential storage.

Expected start output includes:

    [wifi] AP smoke start
    [wifi] no web server, no DNS, no captive portal, no credentials
    [wifi] mode=AP
    [wifi] ap ssid="FarmWhisper-XXXXXX" ip=10.10.10.10

Press `a` again to stop the AP and return WiFi OFF.

The AP smoke test has a 5-minute timeout. If left active, firmware stops the AP automatically and returns WiFi OFF.

### `x` — WiFi status

Prints the current WiFi radio mode and status without changing WiFi state.

Expected idle output:

    [wifi] mode=OFF status=OFF_OR_UNAVAILABLE(255)

### `w` — WiFi scan

Performs one manual WiFi scan.

The command:

1. enables STA mode
2. runs a synchronous scan
3. prints discovered SSIDs, RSSI, channel, and auth state
4. deletes scan results
5. forces WiFi back OFF

Expected final output:

    [wifi] scan done; mode=OFF
    [wifi] mode=OFF status=OFF_OR_UNAVAILABLE(255)

## Safety rules

During this phase, WiFi must remain opt-in and manually triggered.

No code should start WiFi during boot or in the main loop. No captive portal code should be added until the manual radio diagnostics are stable and committed.

## Validation checklist

After any WiFi change:

    Boot banner still appears.
    VL53L1X still detected at 0x29.
    ToF still reports stable/shady/timeout behavior.
    NeoPixel status still works.
    Button short/long/double/triple still works.
    Existing serial commands still work: h, s, i, g, v, r.
    GPIO37/38/39/40 smoke diagnostic still works.
    a toggles AP smoke mode manually.
    x reports WiFi state without changing it.
    AP smoke timeout returns WiFi OFF after 5 minutes.
    w scans only when manually pressed.
    After w completes, WiFi returns to OFF.

## Next intended slices

1. Minimal captive portal page served only after manual command.
2. Minimal captive portal page served only after manual command.
3. Minimal captive portal page served only after manual command.
4. Credential-entry UI.
5. Credential storage.
6. Boot-time connection policy.

## Manual HTTP placeholder

The `a` serial command starts the manual SoftAP smoke/setup mode and, while
that AP is active, a minimal HTTP placeholder server.

Current setup address:

- AP SSID: `FarmWhisper-XXXXXX`
- AP/gateway IP: `10.10.10.10`
- Placeholder page: `http://10.10.10.10/`

This is not a captive portal yet. There is no DNS redirect, credential form,
credential storage, STA connection attempt, or boot-time WiFi behavior in this
slice. The HTTP server is stopped whenever AP smoke/setup mode is stopped,
including the manual `a` stop path and the AP smoke timeout path.

## Manual HTTP placeholder

The `a` serial command starts the manual SoftAP smoke/setup mode and, while
that AP is active, a minimal HTTP placeholder server.

Current setup address:

- AP SSID: `FarmWhisper-XXXXXX`
- AP/gateway IP: `10.10.10.10`
- Placeholder page: `http://10.10.10.10/`

This is not a captive portal yet. There is no DNS redirect, credential form,
credential storage, STA connection attempt, or boot-time WiFi behavior in this
slice. The HTTP server is stopped whenever AP smoke/setup mode is stopped,
including the manual `a` stop path and the AP smoke timeout path.

## Triple press setup AP

A debounced button triple press starts the manual setup AP and placeholder HTTP
server. If setup AP mode is already active, another triple press refreshes the
5-minute AP/setup timeout instead of stopping the AP.

Serial command `a` remains the bench diagnostic toggle. It can still stop the
AP manually.

Future successful setup/config saves should refresh the same 5-minute timer.
Failed saves should not refresh the timer.


## Setup HTTP status endpoint

While the manual setup AP/HTTP mode is active, the firmware serves a small
status endpoint:

- `http://10.10.10.10/status`

The endpoint returns dependency-free JSON with setup/AP state such as whether
AP smoke/setup mode is active, whether setup HTTP is active, AP SSID, AP IP,
station count, AP age, timeout length, and timeout remaining.

This route is read-only. It does not save credentials, change WiFi mode, start
DNS, redirect clients, or perform captive-portal behavior.


## Setup root status page

While the manual setup AP/HTTP mode is active, `http://10.10.10.10/` renders a
simple human-readable setup status page. It shows the same basic AP/setup state
as the JSON endpoint, including AP/setup state, HTTP state, SSID, IP, connected
station count, AP age, timeout length, and timeout remaining.

The root page is server-rendered. There is no JavaScript, no form handling, no
credential entry, no credential storage, no DNS, and no captive-portal redirect
in this slice.


## MAC-derived setup SSID

The default setup AP SSID is derived from the ESP32 WiFi MAC address:

- Format: `FarmWhisper-XXXXXX`
- `XXXXXX` is the last six uppercase hexadecimal characters of the base WiFi
  MAC address.

This gives each device a short deterministic setup name that can be printed on
a label or laser-etched on the case during assembly.

Example:

- MAC suffix: `A1B2C3`
- Setup AP SSID: `FarmWhisper-A1B2C3`

A future user-defined alias may provide a friendly name, but it should not
replace this hardware identity contract.


## Device ID

FarmWhisper derives a short product-facing device ID from the same ESP32 WiFi
MAC suffix used for the setup AP SSID.

- Setup AP SSID: `FarmWhisper-XXXXXX`
- Device ID: `FWP-XXXXXX`
- `XXXXXX`: last six uppercase hexadecimal characters of the ESP32 base WiFi
  MAC address.

The setup root page and `/status` endpoint both report the device ID. This ID
is intended for labels, laser marking, station logs, and future configuration
records.

A future user-defined alias can provide a friendly name such as `Coop Feed
Sensor`, but the alias should not replace the hardware-derived device ID.


## Setup page styling

The setup root page uses a small server-rendered, Windows-98-ish visual style:
high contrast, obvious borders, simple system fonts, and no JavaScript. This
styling does not change WiFi state, routes, credentials, storage, DNS, or
captive-portal behavior.
