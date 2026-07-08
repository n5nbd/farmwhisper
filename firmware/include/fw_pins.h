#pragma once

// FarmWhisper pin contract and hardware notes.
//
// The display I2C pins are reserved for display use only and must not be reused for
// other peripherals or sensors.
//
// The onboard GPIO35 white LED is not a product status indicator and should not be used.
//
// Display support is optional and disabled by default. The firmware should not assume
// that display hardware is present in production.

// Development-board pin contract for the Heltec WiFi LoRa 32 V4.
constexpr int kDisplaySdaPin = 17;          // Display SDA (I2C)
constexpr int kDisplaySclPin = 18;          // Display SCL (I2C)
constexpr int kDisplayResetPin = 21;        // Display reset
constexpr int kDisplayVextPin = 36;         // Display Vext, active LOW
constexpr int kOnboardWhiteLedPin = 35;     // Onboard white LED; not for product status use
constexpr int kDevelopmentButtonPin = 0;    // Development/PRG button input, active LOW

// Proposed physical 8-pin peripheral header block for FarmWhisper.
// This is a physical header-block concept, not a logical signal-order connector.
// The ordering below follows the likely adjacent Heltec V4 header pins and must be
// verified against the physical board before final assignment.
constexpr int kPeripheralHeaderPin1Gnd = -1;          // Physical header position 1: GND
constexpr int kPeripheralHeaderPin2Vdd3v3 = -1;       // Physical header position 2: 3V3
constexpr int kPeripheralHeaderPin3Aux3v3 = -1;       // Physical header position 3: 3V3 / aux 3V3
constexpr int kPeripheralHeaderPin4GpioTbd = -1;      // Physical header position 4: GPIO TBD, likely big button input
constexpr int kPeripheralHeaderPin5SdaGpio45 = 45;    // Physical header position 5: SDA GPIO45, product I2C bus
constexpr int kPeripheralHeaderPin6SclGpio46 = 46;    // Physical header position 6: SCL GPIO46, product I2C bus
constexpr int kPeripheralHeaderPin7GpioTbd = -1;      // Physical header position 7: GPIO TBD, likely NeoPixel data
constexpr int kPeripheralHeaderPin8GpioTbd = -1;      // Physical header position 8: GPIO TBD, spare / interrupt / enable / future use

// Candidate product I2C bus for VL53L1X and similar sensors.
constexpr int kProductI2cSdaPin = 45;
constexpr int kProductI2cSclPin = 46;

// The TBD header positions should avoid bootstrapping, USB, flash/PSRAM, LoRa,
// display, and other reserved or special functions until the physical board is verified.
// Future product-specific pins remain placeholders for now.
// TODO: Assign the product NeoPixel pin when the hardware contract is finalized.
// TODO: Assign the product big button pin when the hardware contract is finalized.
