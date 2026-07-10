#pragma once

#include <stdint.h>

#include "fw_radio_profile.h"

// Dormant FarmWhisper radio integration boundary.
//
// This module records the validated Heltec WiFi LoRa 32 V4 onboard SX1262
// hardware contract and resolves the selected firmware-owned radio profile.
// It does not initialize, transmit, receive, or add a radio library yet.

enum class FwRadioState : uint8_t {
  Disabled = 0,
};

struct FwRadioHardware {
  int8_t nssPin;
  int8_t sckPin;
  int8_t mosiPin;
  int8_t misoPin;
  int8_t resetPin;
  int8_t busyPin;
  int8_t dio1Pin;
};

namespace FWRadio {

const FwRadioHardware &hardware();

const FwRadioProfile *selectedProfile();

FwRadioState state();
const char *stateName();

}  // namespace FWRadio
