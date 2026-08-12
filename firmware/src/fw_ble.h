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

}  // namespace FWBLE
