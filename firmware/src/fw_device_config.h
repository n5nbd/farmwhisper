#pragma once

#include <Arduino.h>

#include "fw_radio_profile.h"
#include "fw_transport_mode.h"

// FarmWhisper product configuration model.
//
// This module owns product-facing configuration fields that are independent of
// the WiFi/setup transport. Device alias, selected radio profile, and selected
// transport mode are stored in NVS.

constexpr size_t kFwDeviceAliasMaxLen = 32;

void fwLoadDeviceConfig();

const char* fwDeviceAlias();
bool fwSetDeviceAlias(const char* alias);
void fwClearDeviceAlias();

FwRadioProfileId fwSelectedRadioProfileId();
bool fwSetSelectedRadioProfileByKey(const char* key);
void fwClearSelectedRadioProfile();

FwTransportModeId fwSelectedTransportModeId();
bool fwSetSelectedTransportModeByKey(const char* key);
void fwClearSelectedTransportMode();

bool fwCalibrationConfigured();
uint16_t fwCalibrationEmptyMm();
uint16_t fwCalibrationFullMm();
bool fwSaveCalibration(uint16_t emptyMm, uint16_t fullMm);
void fwClearCalibration();
