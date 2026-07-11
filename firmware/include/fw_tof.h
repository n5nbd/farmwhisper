#pragma once

/*
 * VL53L1X ToF module.
 *
 * Owns the time-of-flight sensor bring-up and polling path on the product I2C
 * bus. The validated baseline detects the VL53L1X at 0x29 and preserves the
 * last valid distance when later samples are shady or timed out.
 */


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

/*
 * Return the current stability-window average and span only while the ToF
 * owner considers the reading stable.
 */
bool stableReading(uint16_t &avgMm, uint16_t &spanMm);

}  // namespace FWToF
