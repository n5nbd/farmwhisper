#pragma once

#include <Arduino.h>

namespace FWBLE {

// Apply the persisted transport policy and start BLE advertising when enabled.
void begin(Stream &out);

// Reconcile BLE advertising, identity, and configuration values with current
// firmware state. Call from the main loop.
void service(Stream &out);

bool isAdvertising();
const char *advertisedName();

// Force one update of the existing BLE telemetry manufacturer-data packet.
// Used by the XIAO C6 diagnostic flood; normal telemetry scheduling is unchanged.
bool transmitDiagnosticTelemetry(Stream &out);

#if defined(FW_BOARD_XIAO_C6)
// Send one normal scheduled telemetry update immediately using the same
// manufacturer-data encoder and advertising path. Used by timer-wake power
// management so the wake itself is the cadence authority.
bool transmitScheduledTelemetry(Stream &out);

// Account for a planned deep-sleep interval in the existing normal telemetry
// cadence so beacons-per-hour continues to mean wall-clock cadence.
void prepareForDeepSleep(uint32_t sleepMs);

// Keep the node awake briefly after a scheduled telemetry refresh so scanners
// have time to receive the updated manufacturer-data advertisement.
bool normalTelemetryHoldActive();
#endif

}  // namespace FWBLE
