#include "fw_radio.h"

#include "fw_device_config.h"

namespace {

constexpr FwRadioHardware kHeltecV4RadioHardware = {
    8,   // NSS
    9,   // SCK
    10,  // MOSI
    11,  // MISO
    12,  // RESET
    13,  // BUSY
    14,  // DIO1
};

}  // namespace

namespace FWRadio {

const FwRadioHardware &hardware() {
  return kHeltecV4RadioHardware;
}

const FwRadioProfile *selectedProfile() {
  const FwRadioProfile *profile =
      fwRadioProfileById(fwSelectedRadioProfileId());

  return profile == nullptr ? fwDefaultRadioProfile() : profile;
}

FwRadioState state() {
  return FwRadioState::Disabled;
}

const char *stateName() {
  return "DISABLED";
}

}  // namespace FWRadio
