#include "fw_battery.h"

#include <Arduino.h>

#include "fw_pins.h"

namespace {

constexpr uint8_t kSampleCount = 8;
constexpr uint32_t kDividerEnableSettleMs = 2;
constexpr uint32_t kDividerNumerator = 490;
constexpr uint32_t kDividerDenominator = 100;

uint16_t lastBatteryMillivolts = FWBattery::kUnknownMillivolts;

}  // namespace

namespace FWBattery {

void begin() {
  pinMode(FWPin::BatteryMeasureEnable, OUTPUT);
  digitalWrite(FWPin::BatteryMeasureEnable, LOW);
  analogReadResolution(12);
  analogSetPinAttenuation(FWPin::BatteryVoltageAdc, ADC_11db);
}

uint16_t readMillivolts() {
  digitalWrite(FWPin::BatteryMeasureEnable, HIGH);
  delay(kDividerEnableSettleMs);

  uint32_t adcMillivoltTotal = 0;
  for (uint8_t sample = 0; sample < kSampleCount; ++sample) {
    adcMillivoltTotal += analogReadMilliVolts(FWPin::BatteryVoltageAdc);
  }

  digitalWrite(FWPin::BatteryMeasureEnable, LOW);

  const uint32_t adcMillivolts = adcMillivoltTotal / kSampleCount;
  const uint32_t batteryMillivolts =
      (adcMillivolts * kDividerNumerator +
       (kDividerDenominator / 2U)) /
      kDividerDenominator;

  if (batteryMillivolts > 0xFFFEU) {
    lastBatteryMillivolts = kUnknownMillivolts;
  } else {
    lastBatteryMillivolts = static_cast<uint16_t>(batteryMillivolts);
  }

  return lastBatteryMillivolts;
}

void printStatus(Stream &out) {
  const uint16_t batteryMillivolts = readMillivolts();
  out.print("[battery] voltage=");
  if (batteryMillivolts == kUnknownMillivolts) {
    out.println("unknown");
    return;
  }

  out.print(batteryMillivolts / 1000U);
  out.print('.');
  const uint16_t fractional = batteryMillivolts % 1000U;
  if (fractional < 100U) {
    out.print('0');
  }
  if (fractional < 10U) {
    out.print('0');
  }
  out.print(fractional);
  out.println("V");
}

}  // namespace FWBattery
