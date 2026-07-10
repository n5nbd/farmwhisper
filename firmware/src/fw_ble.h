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

}  // namespace FWBLE
