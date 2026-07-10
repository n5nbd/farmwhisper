#pragma once

#include <Arduino.h>

#include "fw_radio_profile.h"

// FarmWhisper product configuration model.
//
// This module owns product-facing configuration fields that are independent of
// the WiFi/setup transport. Device alias and selected radio profile are stored
// in NVS.
constexpr size_t kFwDeviceAliasMaxLen = 32;

void fwLoadDeviceConfig();

const char* fwDeviceAlias();

bool fwSetDeviceAlias(const char* alias);
void fwClearDeviceAlias();

FwRadioProfileId fwSelectedRadioProfileId();
bool fwSetSelectedRadioProfileByKey(const char* key);
void fwClearSelectedRadioProfile();
