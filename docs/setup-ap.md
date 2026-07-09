# FarmWhisper Setup AP and Optional Local PIN

FarmWhisper setup is local-first. The device does not need to join a customer,
home, shop, barn, or RV WiFi network for setup.

Instead, the device can start a temporary setup access point. A phone, laptop,
or tablet connects directly to the FarmWhisper device and opens the setup page.

    phone/laptop/tablet -> FarmWhisper-XXXXXX AP -> http://10.10.10.10/

## Setup AP identity

The setup AP uses a deterministic identity derived from the ESP32 base WiFi MAC.

    Setup SSID: FarmWhisper-XXXXXX
    Device ID:  FWP-XXXXXX

`XXXXXX` is the last six uppercase hex characters of the ESP32 base WiFi MAC.

The setup AP IP/gateway is:

    10.10.10.10

## Setup AP lifecycle

WiFi remains off at boot.

The setup AP/HTTP server starts only when explicitly requested:

    Serial command `a`
    Triple press on the setup button

While the setup AP is active:

    Triple press refreshes the setup timer
    The setup AP auto-stops after 5 minutes

When the setup AP stops, the current setup unlock session is cleared. The stored
PIN state is not changed.

## Setup routes

Routes while setup AP/HTTP is active:

    GET  /           setup page or PIN unlock page
    GET  /setup.css  firmware-served stylesheet
    GET  /status     read-only JSON status
    POST /unlock     unlock setup when a local PIN is configured
    POST /pin        save or clear the optional local setup PIN

The setup UI is server-rendered. It does not rely on JavaScript.

## Optional local setup PIN

The local setup PIN is optional.

This is the intended behavior:

    No PIN stored in NVS:
      / opens setup directly

    PIN stored in NVS:
      / shows the PIN screen first

    Unlocked setup page:
      Save a 6-digit PIN -> store PIN in NVS
      Save a blank PIN   -> clear PIN from NVS

A farmer who does not want to type a PIN does not have to use one. No stored PIN
means no PIN prompt.

If a PIN is used, it must be exactly six digits.

The local PIN is a practical local guardrail against casual setup changes. It is
not meant to be internet-grade security.

## PIN recovery

If the setup PIN is forgotten, use the physical recovery gesture while the setup
AP is active:

    Hold button until LED flashes red five times (~10 seconds).

Recovery behavior:

    Stored setup PIN is cleared from NVS
    Setup opens directly again
    Current setup session is opened
    Setup AP timeout is refreshed
    Status LED flashes red five times

## Serial bench commands

Relevant serial commands:

| Command | Action |
| ------- | ------ |
| `a` | Toggle setup AP/HTTP server |
| `w` | Manual STA WiFi scan, then force WiFi off |
| `x` | Print WiFi/setup status without changing WiFi state |

## Current setup boundaries

Implemented:

    Setup AP starts/stops manually
    Setup AP has deterministic SSID/device ID
    Setup page supports optional local PIN
    PIN changes persist in NVS
    Blank PIN save clears the stored PIN
    Physical recovery clears the stored PIN
    /status exposes setup/PIN policy

Not implemented yet:

    DNS/captive portal redirect
    FarmWhisper product configuration save/apply
    Device alias storage
    Node role storage
    Radio profile storage
    Sensor calibration storage
