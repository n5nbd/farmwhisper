#pragma once

#include <Arduino.h>

namespace FWProductI2C {

void begin();
bool scanFor(uint8_t expectedAddr);

}  // namespace FWProductI2C
