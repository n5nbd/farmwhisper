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
 * - GPIO37: battery-voltage divider enable (dedicated)
 * - GPIO38/39/40: expansion smoke-test GPIOs as INPUT_PULLUP
 */


#include <Arduino.h>

// FarmWhisper Heltec WiFi LoRa 32 V4 R2/R8 base connector contract.
//
// Header J3, bottom-up:
//   1  GND
//   2  3V3
//   3  3V3 / aux 3V3
//   4  GPIO37 battery-voltage divider enable, dedicated
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

#if defined(FW_BOARD_XIAO_C6)

// Tested XIAO ESP32-C6 FW100 wiring.
static constexpr uint8_t ProductI2cSda = 22;  // D4 / GPIO22
static constexpr uint8_t ProductI2cScl = 23;  // D5 / GPIO23
static constexpr uint8_t BigButton = 1;       // D1 / GPIO1, active LOW
static constexpr uint8_t StatusPixel = 2;     // D2 / GPIO2
static constexpr uint8_t SensorRailEnable = 21;  // D3 / GPIO21, LOW = ON
static constexpr uint8_t BatteryVoltageAdc = A0;  // D0/A0 battery divider; use Arduino analog pin mapping

#else

static constexpr uint8_t ProductI2cSda = 45;
static constexpr uint8_t ProductI2cScl = 46;

static constexpr uint8_t BigButton = 42;
static constexpr uint8_t StatusPixel = 41;

static constexpr uint8_t BatteryMeasureEnable = 37;
static constexpr uint8_t BatteryVoltageAdc = 1;
static constexpr uint8_t ExpansionGpio38 = 38;
static constexpr uint8_t ExpansionGpio39 = 39;
static constexpr uint8_t ExpansionGpio40 = 40;

static constexpr uint8_t ExpansionInputs[] = {
    ExpansionGpio38,
    ExpansionGpio39,
    ExpansionGpio40,
};

static constexpr size_t ExpansionInputCount =
    sizeof(ExpansionInputs) / sizeof(ExpansionInputs[0]);

#endif

}  // namespace FWPin
