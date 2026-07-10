#pragma once

#include <Arduino.h>

namespace FWBLE {

// Apply the persisted transport policy at boot.
// BLE-capable modes advertise a connectable, read-only standard Device
// Information Service. No custom FarmWhisper service or data path exists yet.
void begin(Stream &out);

// Reconcile advertising with the current transport mode and device alias.
// Setup-page changes therefore take effect without rebooting.
void service(Stream &out);

bool isAdvertising();
const char *advertisedName();

} // namespace FWBLE
