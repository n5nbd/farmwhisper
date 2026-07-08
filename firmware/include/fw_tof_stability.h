#pragma once

#include <Arduino.h>

namespace FWToFStability {

void addValidSample(uint16_t mm);
bool compute(uint16_t &avgMm, uint16_t &spanMm);
bool isWarming();
void printSummary();
void reset();

}  // namespace FWToFStability
