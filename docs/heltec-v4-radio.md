# Heltec WiFi LoRa 32 V4 radio contract

FarmWhisper currently targets the Heltec WiFi LoRa 32 V4 R2/R8 development
board with its onboard SX1262.

## Validated onboard radio wiring

The Heltec V4.2 schematic shows this ESP32-S3 to SX1262 connection:

| SX1262 signal | ESP32-S3 GPIO |
| --- | ---: |
| NSS | 8 |
| SCK | 9 |
| MOSI | 10 |
| MISO | 11 |
| RESET | 12 |
| BUSY | 13 |
| DIO1 | 14 |

These pins belong to the onboard radio and are not available as general
FarmWhisper product I/O.

## Firmware ownership

`fw_radio_profile.{h,cpp}` owns the firmware-created named radio profiles and
their internal LoRa parameters.

`fw_device_config.{h,cpp}` owns which named profile is selected and persists
that selection in NVS.

`fw_radio.{h,cpp}` is the radio integration boundary. It owns the Heltec V4
radio hardware contract and resolves the selected profile.

## Current state

The radio module is intentionally dormant:

- no RadioLib dependency
- no SX1262 initialization
- no transmit behavior
- no receive behavior
- no change to boot or runtime behavior

The next radio slice can add the driver dependency and a serial-only
initialization diagnostic without changing FarmWhisper application behavior.
