#pragma once

#include <Arduino.h>

#include "fw_types.h"

namespace FWStatusPixel {

void begin();
void update(ComponentStatus componentStatus);
void triggerButtonOverlay(ButtonOverlay overlay, uint32_t durationMs);
void clearButtonOverlay();

}  // namespace FWStatusPixel
