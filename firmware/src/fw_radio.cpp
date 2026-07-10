#include "fw_radio.h"

#include <RadioLib.h>
#include <SPI.h>

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

constexpr float kTcxoVoltage = 1.8f;
constexpr uint8_t kPrivateSyncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
constexpr int8_t kRadioLibSx1262MaxPowerDbm = 22;

Module radioModule(
    kHeltecV4RadioHardware.nssPin,
    kHeltecV4RadioHardware.dio1Pin,
    kHeltecV4RadioHardware.resetPin,
    kHeltecV4RadioHardware.busyPin);

SX1262 radio(&radioModule);

FwRadioState radioState = FwRadioState::Disabled;
int16_t radioResult = RADIOLIB_ERR_NONE;

int8_t appliedPowerDbm(const FwRadioProfile &profile) {
  return profile.txPowerDbm > kRadioLibSx1262MaxPowerDbm
      ? kRadioLibSx1262MaxPowerDbm
      : profile.txPowerDbm;
}

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
  return radioState;
}

const char *stateName() {
  switch (radioState) {
    case FwRadioState::Ready:
      return "READY";
    case FwRadioState::Error:
      return "ERROR";
    case FwRadioState::Disabled:
    default:
      return "DISABLED";
  }
}

int16_t lastResult() {
  return radioResult;
}

void printStatus(Stream &out) {
  const FwRadioProfile *profile = selectedProfile();

  out.print("[radio] state=");
  out.print(stateName());
  out.print(" result=");
  out.print(radioResult);

  if (profile == nullptr) {
    out.println(" profile=none");
    return;
  }

  out.print(" profile=");
  out.print(profile->key);
  out.print(" name=\"");
  out.print(profile->name);
  out.print("\" freqHz=");
  out.print(profile->frequencyHz);
  out.print(" bwHz=");
  out.print(profile->bandwidthHz);
  out.print(" sf=");
  out.print(profile->spreadingFactor);
  out.print(" cr=4/");
  out.print(profile->codingRateDenominator);
  out.print(" preamble=");
  out.print(profile->preambleSymbols);
  out.print(" requestedPowerDbm=");
  out.print(profile->txPowerDbm);
  out.print(" appliedPowerDbm=");
  out.println(appliedPowerDbm(*profile));
}

bool beginDiagnostic(Stream &out) {
  const FwRadioProfile *profile = selectedProfile();

  if (profile == nullptr) {
    radioState = FwRadioState::Error;
    radioResult = RADIOLIB_ERR_UNKNOWN;
    out.println("[radio] init failed: no selected/default profile");
    return false;
  }

  out.println("[radio] explicit SX1262 initialization diagnostic");
  printStatus(out);

  SPI.begin(
      kHeltecV4RadioHardware.sckPin,
      kHeltecV4RadioHardware.misoPin,
      kHeltecV4RadioHardware.mosiPin,
      kHeltecV4RadioHardware.nssPin);

  const float frequencyMhz =
      static_cast<float>(profile->frequencyHz) / 1000000.0f;
  const float bandwidthKhz =
      static_cast<float>(profile->bandwidthHz) / 1000.0f;

  radioResult = radio.begin(
      frequencyMhz,
      bandwidthKhz,
      profile->spreadingFactor,
      profile->codingRateDenominator,
      kPrivateSyncWord,
      appliedPowerDbm(*profile),
      profile->preambleSymbols,
      kTcxoVoltage,
      false);

  if (radioResult != RADIOLIB_ERR_NONE) {
    radioState = FwRadioState::Error;
    out.print("[radio] SX1262 init failed code=");
    out.println(radioResult);
    printStatus(out);
    return false;
  }

  radioResult = radio.setDio2AsRfSwitch(true);
  if (radioResult != RADIOLIB_ERR_NONE) {
    radioState = FwRadioState::Error;
    out.print("[radio] DIO2 RF switch setup failed code=");
    out.println(radioResult);
    printStatus(out);
    return false;
  }

  radioState = FwRadioState::Ready;
  out.println("[radio] SX1262 init PASS; no TX/RX started");
  printStatus(out);
  return true;
}

}  // namespace FWRadio
