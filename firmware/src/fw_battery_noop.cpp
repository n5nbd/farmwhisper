#if defined(FW_BOARD_XIAO_C6)

#include "fw_battery.h"

#include <Arduino.h>

#include "fw_pins.h"

namespace {

constexpr uint8_t kDiscardCount = 4;
constexpr uint8_t kSampleCount = 16;
constexpr uint32_t kBatteryMultiplierPpm = 2066667UL;  // (176k + 165k) / 165k

uint16_t lastBatteryMillivolts = FWBattery::kUnknownMillivolts;

uint16_t readBatteryPinMillivolts() {
  for (uint8_t discard = 0; discard < kDiscardCount; ++discard) {
    (void)analogReadMilliVolts(FWPin::BatteryVoltageAdc);
    delay(2);
  }

  uint32_t totalMillivolts = 0;
  for (uint8_t sample = 0; sample < kSampleCount; ++sample) {
    totalMillivolts += analogReadMilliVolts(FWPin::BatteryVoltageAdc);
    delay(2);
  }

  return static_cast<uint16_t>((totalMillivolts + (kSampleCount / 2U)) /
                               kSampleCount);
}

}  // namespace

namespace FWBattery {

void begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(FWPin::BatteryVoltageAdc, ADC_11db);
}

uint16_t readMillivolts() {
  const uint16_t pinMillivolts = readBatteryPinMillivolts();
  const uint64_t scaled =
      static_cast<uint64_t>(pinMillivolts) * kBatteryMultiplierPpm;
  const uint32_t batteryMillivolts =
      static_cast<uint32_t>((scaled + 500000ULL) / 1000000ULL);

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
  if (fractional < 100U) out.print('0');
  if (fractional < 10U) out.print('0');
  out.print(fractional);
  out.println("V");
}

}  // namespace FWBattery

#endif
