#include "fw_tof.h"

#include <VL53L1X.h>

#include "fw_config.h"
#include "fw_tof_stability.h"
#include "fw_types.h"

namespace {

VL53L1X tof;

ComponentStatus componentStatus = ComponentStatus::Booting;

bool tofReady = false;
bool tofVerboseLogging = false;
uint32_t tofValidCount = 0;
uint32_t tofShadyCount = 0;
uint32_t tofTimeoutCount = 0;
uint16_t lastValidTofMm = 0;
bool hasLastValidTof = false;

uint32_t lastTofPollMs = 0;

}  // namespace

namespace FWToF {

bool begin(bool foundOnI2cScan) {
  Serial.println();
  Serial.println("[tof] Initializing VL53L1X");

  tof.setTimeout(500);

  if (!foundOnI2cScan || !tof.init()) {
    tofReady = false;
    componentStatus = ComponentStatus::TofInitFailed;
    Serial.println("[tof] ERROR: VL53L1X init failed");
    return false;
  }

  tof.setDistanceMode(VL53L1X::Long);
  tof.setMeasurementTimingBudget(50000);
  tof.startContinuous(100);
  lastTofPollMs = millis();

  tofReady = true;
  componentStatus = ComponentStatus::TofWarming;

  Serial.println("[tof] VL53L1X ready");
  Serial.println("[tof] Mode=Long timingBudgetUs=50000 continuousPeriodMs=100");

  return true;
}

void poll() {
  if (!tofReady) {
    componentStatus = ComponentStatus::TofInitFailed;
    return;
  }

  const uint32_t now = millis();
  if ((now - lastTofPollMs) < FWConfig::TofPollMs) {
    return;
  }
  lastTofPollMs = now;

  const uint16_t distanceMm = tof.read();

  if (tof.timeoutOccurred()) {
    tofTimeoutCount++;
    componentStatus = ComponentStatus::TofTimeout;

    if (tofVerboseLogging) {
      Serial.print("[tof] timeout timeoutCount=");
      Serial.print(tofTimeoutCount);
      Serial.print(" lastValidMm=");
      if (hasLastValidTof) {
        Serial.print(lastValidTofMm);
      } else {
        Serial.print("none");
      }
      FWToFStability::printSummary();
      Serial.println();
    }

    return;
  }

  const VL53L1X::RangeStatus rangeStatus = tof.ranging_data.range_status;
  const char *rangeStatusName = VL53L1X::rangeStatusToString(rangeStatus);

  if (rangeStatus != VL53L1X::RangeValid) {
    tofShadyCount++;
    componentStatus = ComponentStatus::TofShady;

    if (tofVerboseLogging) {
      Serial.print("[tof] ~");
      Serial.print(distanceMm);
      Serial.print(" mm status=");
      Serial.print(rangeStatusName);
      Serial.print(" shadyCount=");
      Serial.print(tofShadyCount);
      Serial.print(" lastValidMm=");
      if (hasLastValidTof) {
        Serial.print(lastValidTofMm);
      } else {
        Serial.print("none");
      }
      FWToFStability::printSummary();
      Serial.println();
    }

    return;
  }

  tofValidCount++;
  lastValidTofMm = distanceMm;
  hasLastValidTof = true;
  FWToFStability::addValidSample(distanceMm);

  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = FWToFStability::compute(avgMm, spanMm);

  if (FWToFStability::isWarming()) {
    componentStatus = ComponentStatus::TofWarming;
  } else if (stable) {
    componentStatus = ComponentStatus::TofStable;
  } else {
    componentStatus = ComponentStatus::TofUnstable;
  }

  if (tofVerboseLogging) {
    Serial.print("[tof] ");
    Serial.print(distanceMm);
    Serial.print(" mm status=");
    Serial.print(rangeStatusName);
    Serial.print(" validCount=");
    Serial.print(tofValidCount);
    Serial.print(" lastValidMm=");
    Serial.print(lastValidTofMm);
    FWToFStability::printSummary();
    Serial.println();
  }
}

void resetDiagnostics() {
  tofValidCount = 0;
  tofShadyCount = 0;
  tofTimeoutCount = 0;
  lastValidTofMm = 0;
  hasLastValidTof = false;

  FWToFStability::reset();

  if (tofReady) {
    componentStatus = ComponentStatus::TofWarming;
  } else {
    componentStatus = ComponentStatus::TofInitFailed;
  }
}

ComponentStatus status() {
  return componentStatus;
}

bool ready() {
  return tofReady;
}

bool verboseLogging() {
  return tofVerboseLogging;
}

void toggleVerboseLogging() {
  tofVerboseLogging = !tofVerboseLogging;
}

uint32_t validCount() {
  return tofValidCount;
}

uint32_t shadyCount() {
  return tofShadyCount;
}

uint32_t timeoutCount() {
  return tofTimeoutCount;
}

bool hasLastValid() {
  return hasLastValidTof;
}

uint16_t lastValidMm() {
  return lastValidTofMm;
}

bool stableReading(uint16_t &avgMm, uint16_t &spanMm) {
  avgMm = 0;
  spanMm = 0;

  if (componentStatus != ComponentStatus::TofStable) {
    return false;
  }

  return FWToFStability::compute(avgMm, spanMm);
}

}  // namespace FWToF
