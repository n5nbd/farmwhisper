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
    x reports WiFi state without changing it.
    w scans only when manually pressed.
    After w completes, WiFi returns to OFF.

## Next intended slices

1. Manual AP-mode smoke test.
2. Manual AP off command or AP timeout.
3. Minimal captive portal page served only after manual command.
4. Credential-entry UI.
5. Credential storage.
6. Boot-time connection policy.
