#pragma once

/*
 * Status-pixel module.
 *
 * Owns the GPIO41 NeoPixel behavior used during component validation. This
 * module should remain small and hardware-focused: higher-level application
 * state should request colors/patterns rather than directly touching the LED.
 */


#include <Arduino.h>

#include "fw_types.h"

namespace FWStatusPixel {

void begin();
void update(ComponentStatus componentStatus);
void triggerButtonOverlay(ButtonOverlay overlay, uint32_t durationMs);
void clearButtonOverlay();

}  // namespace FWStatusPixel
