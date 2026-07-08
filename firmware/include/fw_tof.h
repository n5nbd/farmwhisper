#pragma once

#include <Arduino.h>

#include "fw_types.h"

namespace FWToF {

bool begin(bool foundOnI2cScan);
void poll();
void resetDiagnostics();

ComponentStatus status();

bool ready();
bool verboseLogging();
void toggleVerboseLogging();

uint32_t validCount();
uint32_t shadyCount();
uint32_t timeoutCount();

bool hasLastValid();
uint16_t lastValidMm();

}  // namespace FWToF
