#pragma once

#include <Arduino.h>

// Read-only scaffold for future FarmWhisper product configuration.
//
// This intentionally does not persist anything yet. The first real field is a
// device alias, but for this slice it is only a compiled-in default so the model
// can be introduced without changing setup behavior.
const char* fwDeviceAlias();
