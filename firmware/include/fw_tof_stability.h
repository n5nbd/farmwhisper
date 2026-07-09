#pragma once

/*
 * ToF stability module.
 *
 * Owns classification of VL53L1X samples into stable, shady, timeout, and
 * last-valid app-facing state. The leading '~' convention means a value is
 * questionable/stale/shady and must not be presented as clean truth.
 */


#include <Arduino.h>

namespace FWToFStability {

void addValidSample(uint16_t mm);
bool compute(uint16_t &avgMm, uint16_t &spanMm);
bool isWarming();
void printSummary();
void reset();

}  // namespace FWToFStability
