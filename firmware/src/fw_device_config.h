#pragma once

#include <Arduino.h>

// FarmWhisper product configuration model.
//
// This module owns product-facing configuration fields that are independent of
// the WiFi/setup transport. Values are NVS-backed, but this slice only reads and
// exposes the device alias. Setup UI editing comes later.
constexpr size_t kFwDeviceAliasMaxLen = 32;

void fwLoadDeviceConfig();

const char* fwDeviceAlias();

bool fwSetDeviceAlias(const char* alias);
void fwClearDeviceAlias();
