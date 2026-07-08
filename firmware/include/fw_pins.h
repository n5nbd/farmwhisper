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
// The ordering below follows the observed Heltec V4 Header J3 bottom-up order on both
// R2 and R8 pinouts and must be verified against the physical board before final assignment.
constexpr int FW_I2C_SDA = 45;                          // Product I2C SDA (GPIO45)
constexpr int FW_I2C_SCL = 46;                          // Product I2C SCL (GPIO46)
constexpr int FW_CONNECTOR_PIN4_GPIO = 37;              // Physical header position 4: GPIO37 spare / TBD
constexpr int FW_BIG_BUTTON_CANDIDATE = 42;             // Physical header position 7: GPIO42 big button candidate
constexpr int FW_NEOPIXEL_CANDIDATE = 41;              // Physical header position 8: GPIO41 NeoPixel data candidate
constexpr int FW_EXPANSION_GPIO40 = 40;                 // Optional expansion pin 9: GPIO40
constexpr int FW_EXPANSION_GPIO39 = 39;                 // Optional expansion pin 10: GPIO39
constexpr int FW_EXPANSION_GPIO38 = 38;                 // Optional expansion pin 11: GPIO38

constexpr int kPeripheralHeaderPin1Gnd = -1;           // Physical header position 1: GND
constexpr int kPeripheralHeaderPin2Vdd3v3 = -1;        // Physical header position 2: 3V3
constexpr int kPeripheralHeaderPin3Aux3v3 = -1;        // Physical header position 3: 3V3 / aux 3V3
constexpr int kPeripheralHeaderPin4Gpio37 = FW_CONNECTOR_PIN4_GPIO;
constexpr int kPeripheralHeaderPin5SclGpio46 = FW_I2C_SCL;
constexpr int kPeripheralHeaderPin6SdaGpio45 = FW_I2C_SDA;
constexpr int kPeripheralHeaderPin7Gpio42 = FW_BIG_BUTTON_CANDIDATE;
constexpr int kPeripheralHeaderPin8Gpio41 = FW_NEOPIXEL_CANDIDATE;

// GPIO45/GPIO46 have ESP32-S3 boot/strapping sensitivity, so external circuitry must
// not strongly drive or load them during boot. They remain the product I2C bus because
// they were verified working on the Heltec V4 ToF mule.
// GPIO41/GPIO42 may be labeled for GNSS PPS/reset/control on Heltec pinmaps, so they
// must not be used for FarmWhisper product I/O on any board build where GNSS is populated
// and expected to function.
// FarmWhisper base nodes will not use GNSS/GPS by default, so GNSS-associated pins may be
// claimed for product I/O when GNSS is not populated or used.
