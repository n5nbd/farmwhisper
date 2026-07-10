#include "fw_device_config.h"

#include <Preferences.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {
constexpr const char* kConfigNamespace = "fwdevcfg";
constexpr const char* kAliasKey = "alias";
constexpr const char* kDefaultDeviceAlias = "FarmWhisper device";

bool configLoaded = false;
char deviceAlias[kFwDeviceAliasMaxLen + 1] = "";

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

} // namespace

void fwLoadDeviceConfig() {
  Preferences prefs;
  copyAlias(kDefaultDeviceAlias);

  if (prefs.begin(kConfigNamespace, true)) {
    String storedAlias = prefs.getString(kAliasKey, kDefaultDeviceAlias);
    if (isValidAlias(storedAlias.c_str())) {
      copyAlias(storedAlias.c_str());
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
  return FwRadioProfileId::UsDefault;
}
