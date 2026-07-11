#pragma once

#include <stddef.h>
#include <stdint.h>

enum class FwRadioProfileId : uint8_t {
  UsDefault = 0,
  UsLongRange = 1,
};

struct FwRadioProfile {
  FwRadioProfileId id;
  const char *key;
  const char *name;

  // Internal radio parameters. These are firmware-owned and should not be
  // presented to users as editable raw LoRa knobs.
  int8_t txPowerDbm;
  uint32_t bandwidthHz;
  uint8_t spreadingFactor;
  uint8_t codingRateDenominator;
  uint16_t preambleSymbols;
};

size_t fwRadioProfileCount();
const FwRadioProfile *fwRadioProfileAt(size_t index);

const FwRadioProfile *fwDefaultRadioProfile();
const FwRadioProfile *fwRadioProfileById(FwRadioProfileId id);
const FwRadioProfile *fwRadioProfileByKey(const char *key);

const char *fwRadioProfileKey(FwRadioProfileId id);
const char *fwRadioProfileName(FwRadioProfileId id);
