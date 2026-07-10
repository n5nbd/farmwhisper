#include "fw_device_config.h"

#include <Preferences.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr const char* kConfigNamespace = "fwdevcfg";
constexpr const char* kAliasKey = "alias";
constexpr const char* kRadioProfileKey = "radioProfile";
constexpr const char* kDefaultDeviceAlias = "FarmWhisper device";

bool configLoaded = false;
char deviceAlias[kFwDeviceAliasMaxLen + 1] = "";
FwRadioProfileId selectedRadioProfileId = FwRadioProfileId::UsDefault;

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

}  // namespace

void fwLoadDeviceConfig() {
  Preferences prefs;

  copyAlias(kDefaultDeviceAlias);
  useDefaultRadioProfile();

  if (prefs.begin(kConfigNamespace, true)) {
    const String storedAlias =
        prefs.getString(kAliasKey, kDefaultDeviceAlias);

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
