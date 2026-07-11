#include "fw_device_config.h"

#include <Preferences.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr const char* kConfigNamespace = "fwdevcfg";
constexpr const char* kAliasKey = "alias";
constexpr const char* kRadioProfileKey = "radioProfile";
constexpr const char* kTransportModeKey = "transportMode";
constexpr const char* kCalibrationKey = "calibration";
constexpr const char* kDefaultDeviceAlias = "FarmWhisper device";
constexpr uint16_t kCalibrationStorageVersion = 1;

struct StoredCalibration {
  uint16_t version;
  uint16_t emptyMm;
  uint16_t fullMm;
};

bool configLoaded = false;
char deviceAlias[kFwDeviceAliasMaxLen + 1] = "";
FwRadioProfileId selectedRadioProfileId = FwRadioProfileId::UsDefault;
FwTransportModeId selectedTransportModeId = FwTransportModeId::LoRa;
bool calibrationConfigured = false;
uint16_t calibrationEmptyMm = 0;
uint16_t calibrationFullMm = 0;

void copyAlias(const char* value) {
  if (value == nullptr || value[0] == '\0') {
    snprintf(deviceAlias, sizeof(deviceAlias), "%s", kDefaultDeviceAlias);
    return;
  }

  snprintf(deviceAlias, sizeof(deviceAlias), "%s", value);
}

bool isValidAlias(const char* value) {
  if (value == nullptr) {
    return false;
  }

  const size_t len = strlen(value);
  if (len == 0 || len > kFwDeviceAliasMaxLen) {
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    const unsigned char c = static_cast<unsigned char>(value[i]);
    if (iscntrl(c)) {
      return false;
    }
  }

  return true;
}

void useDefaultRadioProfile() {
  const FwRadioProfile* profile = fwDefaultRadioProfile();
  selectedRadioProfileId =
      profile == nullptr ? FwRadioProfileId::UsDefault : profile->id;
}

void useDefaultTransportMode() {
  const FwTransportMode* mode = fwDefaultTransportMode();
  selectedTransportModeId =
      mode == nullptr ? FwTransportModeId::LoRa : mode->id;
}

}  // namespace

void fwLoadDeviceConfig() {
  Preferences prefs;

  copyAlias(kDefaultDeviceAlias);
  useDefaultRadioProfile();
  useDefaultTransportMode();
  calibrationConfigured = false;
  calibrationEmptyMm = 0;
  calibrationFullMm = 0;

  if (prefs.begin(kConfigNamespace, true)) {
    const String storedAlias = prefs.getString(kAliasKey, kDefaultDeviceAlias);
    if (isValidAlias(storedAlias.c_str())) {
      copyAlias(storedAlias.c_str());
    }

    const String storedRadioProfileKey =
        prefs.getString(kRadioProfileKey, "");
    const FwRadioProfile* storedRadioProfile =
        fwRadioProfileByKey(storedRadioProfileKey.c_str());
    if (storedRadioProfile != nullptr) {
      selectedRadioProfileId = storedRadioProfile->id;
    }

    const String storedTransportModeKey =
        prefs.getString(kTransportModeKey, "");
    const FwTransportMode* storedTransportMode =
        fwTransportModeByKey(storedTransportModeKey.c_str());
    if (storedTransportMode != nullptr) {
      selectedTransportModeId = storedTransportMode->id;
    }

    if (prefs.getBytesLength(kCalibrationKey) ==
        sizeof(StoredCalibration)) {
      StoredCalibration storedCalibration = {};
      if (prefs.getBytes(
              kCalibrationKey,
              &storedCalibration,
              sizeof(storedCalibration)) == sizeof(storedCalibration) &&
          storedCalibration.version == kCalibrationStorageVersion &&
          storedCalibration.emptyMm > storedCalibration.fullMm) {
        calibrationEmptyMm = storedCalibration.emptyMm;
        calibrationFullMm = storedCalibration.fullMm;
        calibrationConfigured = true;
      }
    }

    prefs.end();
  }

  configLoaded = true;
}

const char* fwDeviceAlias() {
  if (!configLoaded) {
    fwLoadDeviceConfig();
  }

  return deviceAlias;
}

bool fwSetDeviceAlias(const char* alias) {
  if (!isValidAlias(alias)) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kConfigNamespace, false)) {
    return false;
  }

  const bool ok = prefs.putString(kAliasKey, alias) > 0;
  prefs.end();

  if (ok) {
    copyAlias(alias);
    configLoaded = true;
  }

  return ok;
}

void fwClearDeviceAlias() {
  Preferences prefs;
  if (prefs.begin(kConfigNamespace, false)) {
    prefs.remove(kAliasKey);
    prefs.end();
  }

  copyAlias(kDefaultDeviceAlias);
  configLoaded = true;
}

FwRadioProfileId fwSelectedRadioProfileId() {
  if (!configLoaded) {
    fwLoadDeviceConfig();
  }

  return selectedRadioProfileId;
}

bool fwSetSelectedRadioProfileByKey(const char* key) {
  const FwRadioProfile* profile = fwRadioProfileByKey(key);
  if (profile == nullptr) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kConfigNamespace, false)) {
    return false;
  }

  const bool ok = prefs.putString(kRadioProfileKey, profile->key) > 0;
  prefs.end();

  if (ok) {
    selectedRadioProfileId = profile->id;
    configLoaded = true;
  }

  return ok;
}

void fwClearSelectedRadioProfile() {
  Preferences prefs;
  if (prefs.begin(kConfigNamespace, false)) {
    prefs.remove(kRadioProfileKey);
    prefs.end();
  }

  useDefaultRadioProfile();
  configLoaded = true;
}

FwTransportModeId fwSelectedTransportModeId() {
  if (!configLoaded) {
    fwLoadDeviceConfig();
  }

  return selectedTransportModeId;
}

bool fwSetSelectedTransportModeByKey(const char* key) {
  const FwTransportMode* mode = fwTransportModeByKey(key);
  if (mode == nullptr) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kConfigNamespace, false)) {
    return false;
  }

  const bool ok = prefs.putString(kTransportModeKey, mode->key) > 0;
  prefs.end();

  if (ok) {
    selectedTransportModeId = mode->id;
    configLoaded = true;
  }

  return ok;
}

void fwClearSelectedTransportMode() {
  Preferences prefs;
  if (prefs.begin(kConfigNamespace, false)) {
    prefs.remove(kTransportModeKey);
    prefs.end();
  }

  useDefaultTransportMode();
  configLoaded = true;
}


bool fwCalibrationConfigured() {
  if (!configLoaded) {
    fwLoadDeviceConfig();
  }

  return calibrationConfigured;
}

uint16_t fwCalibrationEmptyMm() {
  if (!configLoaded) {
    fwLoadDeviceConfig();
  }

  return calibrationConfigured ? calibrationEmptyMm : 0;
}

uint16_t fwCalibrationFullMm() {
  if (!configLoaded) {
    fwLoadDeviceConfig();
  }

  return calibrationConfigured ? calibrationFullMm : 0;
}

bool fwSaveCalibration(uint16_t emptyMm, uint16_t fullMm) {
  if (emptyMm <= fullMm) {
    return false;
  }

  const StoredCalibration storedCalibration = {
      kCalibrationStorageVersion,
      emptyMm,
      fullMm,
  };

  Preferences prefs;
  if (!prefs.begin(kConfigNamespace, false)) {
    return false;
  }

  const bool ok =
      prefs.putBytes(
          kCalibrationKey,
          &storedCalibration,
          sizeof(storedCalibration)) == sizeof(storedCalibration);
  prefs.end();

  if (ok) {
    calibrationEmptyMm = emptyMm;
    calibrationFullMm = fullMm;
    calibrationConfigured = true;
    configLoaded = true;
  }

  return ok;
}

void fwClearCalibration() {
  Preferences prefs;
  if (prefs.begin(kConfigNamespace, false)) {
    prefs.remove(kCalibrationKey);
    prefs.end();
  }

  calibrationEmptyMm = 0;
  calibrationFullMm = 0;
  calibrationConfigured = false;
  configLoaded = true;
}
