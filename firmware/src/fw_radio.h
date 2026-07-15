#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "fw_radio_channel.h"
#include "fw_radio_profile.h"

// FarmWhisper SX1262 integration boundary.
//
// The radio remains inactive at boot. Explicit diagnostics initialize the
// onboard Heltec V4 SX1262 using the selected firmware-owned profile. The
// bounded diagnostic burst is serviced without blocking the main loop.

enum class FwRadioState : uint8_t {
  Disabled = 0,
  Ready,
  Error,
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
const FwRadioProfile *appliedProfile();
const FwRadioChannel *selectedChannel();
const FwRadioChannel *appliedChannel();

const char *selectedProfileKey();
const char *appliedProfileKey();
const char *selectedChannelKey();
const char *appliedChannelKey();

FwRadioState state();
const char *stateName();
int16_t lastResult();

bool beginDiagnostic(Stream &out);
bool transmitDiagnostic(Stream &out);

bool startDiagnosticBurst(Stream &out);
void serviceDiagnosticBurst(Stream &out);
void cancelDiagnosticBurst(Stream &out, const char *reason);
bool diagnosticBurstActive();

void serviceNormalBeacon(Stream &out);
void resetNormalBeaconSchedule();

uint32_t txCount();
int16_t lastTxResult();

void printStatus(Stream &out);

} // namespace FWRadio
