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

Toggles a temporary open SoftAP named `FarmWhisper-Setup` at `10.10.10.10`.

This is only a radio smoke test. It does not start a web server, DNS server, captive portal, credential UI, or credential storage.

Expected start output includes:

    [wifi] AP smoke start
    [wifi] no web server, no DNS, no captive portal, no credentials
    [wifi] mode=AP
    [wifi] ap ssid="FarmWhisper-Setup" ip=10.10.10.10

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

- AP SSID: `FarmWhisper-Setup`
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

- AP SSID: `FarmWhisper-Setup`
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
