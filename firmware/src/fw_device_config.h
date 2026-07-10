#pragma once

#include <Arduino.h>

#include "fw_radio_profile.h"

// FarmWhisper product configuration model.
//
// This module owns product-facing configuration fields that are independent of
// the WiFi/setup transport. The device alias is NVS-backed. Radio profile
// selection is exposed here as product configuration but is not persistent yet.
constexpr size_t kFwDeviceAliasMaxLen = 32;

void fwLoadDeviceConfig();

const char* fwDeviceAlias();

bool fwSetDeviceAlias(const char* alias);
void fwClearDeviceAlias();

FwRadioProfileId fwSelectedRadioProfileId();
