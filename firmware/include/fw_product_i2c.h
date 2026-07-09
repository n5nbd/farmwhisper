#pragma once

/*
 * Product I2C module.
 *
 * Owns the GPIO45/GPIO46 product I2C bus used by external FarmWhisper sensors.
 * The display I2C bus, when present on a board, is reserved for display-only
 * use and must not be reused for product peripherals.
 */


#include <Arduino.h>

namespace FWProductI2C {

void begin();
bool scanFor(uint8_t expectedAddr);

}  // namespace FWProductI2C
