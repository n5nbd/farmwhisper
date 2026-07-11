#include "fw_radio_profile.h"

#include <string.h>

namespace {

constexpr FwRadioProfile kRadioProfiles[] = {
    {
        FwRadioProfileId::UsDefault,
        "us-default",
        "US default",
        23,
        500000UL,
        7,
        5,
        8,
    },
    {
        FwRadioProfileId::UsLongRange,
        "us-long-range",
        "US long range",
        23,
        125000UL,
        10,
        5,
        12,
    },
};

constexpr size_t kRadioProfileCount =
    sizeof(kRadioProfiles) / sizeof(kRadioProfiles[0]);

}  // namespace

size_t fwRadioProfileCount() {
  return kRadioProfileCount;
}

const FwRadioProfile *fwRadioProfileAt(size_t index) {
  if (index >= kRadioProfileCount) {
    return nullptr;
  }

  return &kRadioProfiles[index];
}

const FwRadioProfile *fwDefaultRadioProfile() {
  return &kRadioProfiles[0];
}

const FwRadioProfile *fwRadioProfileById(FwRadioProfileId id) {
  for (size_t i = 0; i < kRadioProfileCount; ++i) {
    if (kRadioProfiles[i].id == id) {
      return &kRadioProfiles[i];
    }
  }

  return nullptr;
}

const FwRadioProfile *fwRadioProfileByKey(const char *key) {
  if (key == nullptr || key[0] == '\0') {
    return nullptr;
  }

  for (size_t i = 0; i < kRadioProfileCount; ++i) {
    if (strcmp(kRadioProfiles[i].key, key) == 0) {
      return &kRadioProfiles[i];
    }
  }

  return nullptr;
}

const char *fwRadioProfileKey(FwRadioProfileId id) {
  const FwRadioProfile *profile = fwRadioProfileById(id);
  return profile == nullptr ? "" : profile->key;
}

const char *fwRadioProfileName(FwRadioProfileId id) {
  const FwRadioProfile *profile = fwRadioProfileById(id);
  return profile == nullptr ? "" : profile->name;
}
