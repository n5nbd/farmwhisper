#pragma once

/*
 * FarmWhisper validated hardware pin contract.
 *
 * These assignments describe the Heltec WiFi LoRa 32 V4 R2/R8 component-
 * validation baseline. Do not change pin ownership casually: several pins
 * have already been hardware-regression-tested against the FW100 bring-up
 * fixture.
 *
 * Known validated interfaces:
 * - GPIO41: one NeoPixel/status pixel
 * - GPIO42: active-LOW user button
 * - GPIO45/GPIO46: product I2C bus
 * - GPIO37/38/39/40: spare expansion smoke-test GPIOs as INPUT_PULLUP
 */


#include <Arduino.h>

// FarmWhisper Heltec WiFi LoRa 32 V4 R2/R8 base connector contract.
//
// Header J3, bottom-up:
//   1  GND
//   2  3V3
//   3  3V3 / aux 3V3
//   4  GPIO37 spare/questionable GPIO, validated INPUT_PULLUP input
//   5  GPIO46 product I2C SCL
//   6  GPIO45 product I2C SDA
//   7  GPIO42 big button, active LOW, verified
//   8  GPIO41 NeoPixel data, verified
//
// Optional expansion:
//   9   GPIO40, validated INPUT_PULLUP input
//   10  GPIO39, validated INPUT_PULLUP input
//   11  GPIO38, validated INPUT_PULLUP input

namespace FWPin {

static constexpr uint8_t ProductI2cSda = 45;
static constexpr uint8_t ProductI2cScl = 46;

static constexpr uint8_t BigButton = 42;
static constexpr uint8_t StatusPixel = 41;

static constexpr uint8_t SpareGpio37 = 37;
static constexpr uint8_t ExpansionGpio38 = 38;
static constexpr uint8_t ExpansionGpio39 = 39;
static constexpr uint8_t ExpansionGpio40 = 40;

static constexpr uint8_t ExpansionInputs[] = {
    SpareGpio37,
    ExpansionGpio38,
    ExpansionGpio39,
    ExpansionGpio40,
};

static constexpr size_t ExpansionInputCount =
    sizeof(ExpansionInputs) / sizeof(ExpansionInputs[0]);

}  // namespace FWPin
