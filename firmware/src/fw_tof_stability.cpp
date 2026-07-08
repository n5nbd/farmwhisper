#include "fw_tof_stability.h"

#include "fw_config.h"

namespace {

uint16_t stabilityWindow[FWConfig::StabilityWindowSize] = {};
uint8_t stabilityCount = 0;
uint8_t stabilityWriteIndex = 0;

}  // namespace

namespace FWToFStability {

void addValidSample(uint16_t mm) {
  stabilityWindow[stabilityWriteIndex] = mm;
  stabilityWriteIndex = (stabilityWriteIndex + 1) % FWConfig::StabilityWindowSize;

  if (stabilityCount < FWConfig::StabilityWindowSize) {
    stabilityCount++;
  }
}

bool compute(uint16_t &avgMm, uint16_t &spanMm) {
  if (stabilityCount == 0) {
    avgMm = 0;
    spanMm = 0;
    return false;
  }

  uint32_t sum = 0;
  uint16_t minMm = stabilityWindow[0];
  uint16_t maxMm = stabilityWindow[0];

  for (uint8_t i = 0; i < stabilityCount; i++) {
    const uint16_t value = stabilityWindow[i];
    sum += value;

    if (value < minMm) {
      minMm = value;
    }

    if (value > maxMm) {
      maxMm = value;
    }
  }

  avgMm = static_cast<uint16_t>(sum / stabilityCount);
  spanMm = maxMm - minMm;

  return stabilityCount == FWConfig::StabilityWindowSize && spanMm <= FWConfig::StabilityMaxSpanMm;
}

bool isWarming() {
  return stabilityCount < FWConfig::StabilityWindowSize;
}

void printSummary() {
  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = compute(avgMm, spanMm);

  Serial.print(" stable=");
  if (isWarming()) {
    Serial.print("warming");
  } else {
    Serial.print(stable ? "yes" : "no");
  }

  Serial.print(" stableAvgMm=");
  Serial.print(avgMm);
  Serial.print(" stableSpanMm=");
  Serial.print(spanMm);
}

void reset() {
  for (uint8_t i = 0; i < FWConfig::StabilityWindowSize; i++) {
    stabilityWindow[i] = 0;
  }

  stabilityCount = 0;
  stabilityWriteIndex = 0;
}

}  // namespace FWToFStability
