#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace FWBattery {

constexpr uint16_t kUnknownMillivolts = 0xFFFFU;

void begin();
uint16_t readMillivolts();
void printStatus(Stream &out);

}  // namespace FWBattery
