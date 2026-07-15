#include "fw_radio.h"

#include <RadioLib.h>
#include <SPI.h>
#include <string.h>

#include "fw_device_config.h"
#include "fw_packet.h"
#include "fw_tof.h"

namespace {

constexpr FwRadioHardware kHeltecV4RadioHardware = {
    8,  // NSS
    9,  // SCK
    10, // MOSI
    11, // MISO
    12, // RESET
    13, // BUSY
    14, // DIO1
};

constexpr float kTcxoVoltage = 1.8f;
constexpr uint8_t kPrivateSyncWord =
    RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
constexpr int8_t kRadioLibSx1262MaxPowerDbm = 22;

constexpr uint32_t kDiagnosticBurstDurationMs =
    3UL * 60UL * 1000UL;
constexpr uint32_t kDiagnosticBurstIntervalMs = 5UL * 1000UL;
constexpr uint32_t kDiagnosticBurstExpectedPackets =
    kDiagnosticBurstDurationMs / kDiagnosticBurstIntervalMs;

Module radioModule(
    kHeltecV4RadioHardware.nssPin,
    kHeltecV4RadioHardware.dio1Pin,
    kHeltecV4RadioHardware.resetPin,
    kHeltecV4RadioHardware.busyPin);

SX1262 radio(&radioModule);

FwRadioState radioState = FwRadioState::Disabled;
int16_t radioResult = RADIOLIB_ERR_NONE;
uint32_t radioTxCount = 0;
int16_t radioLastTxResult = RADIOLIB_ERR_NONE;

bool appliedProfileValid = false;
FwRadioProfileId appliedProfileId = FwRadioProfileId::UsDefault;
bool appliedChannelValid = false;
uint8_t appliedChannelNumber = 9;

bool diagnosticBurstRunning = false;
uint32_t diagnosticBurstStartedAtMs = 0;
uint32_t diagnosticBurstNextTxAtMs = 0;
uint32_t diagnosticBurstAttemptCount = 0;
uint32_t diagnosticBurstPassCount = 0;
uint32_t diagnosticBurstFailureCount = 0;

int8_t appliedPowerDbm(const FwRadioProfile &profile) {
  return profile.txPowerDbm > kRadioLibSx1262MaxPowerDbm
      ? kRadioLibSx1262MaxPowerDbm
      : profile.txPowerDbm;
}

uint32_t diagnosticBurstElapsedMs(uint32_t now) {
  if (!diagnosticBurstRunning) {
    return 0;
  }

  return now - diagnosticBurstStartedAtMs;
}

void printDiagnosticBurstSummary(
    Stream &out,
    const char *stateText,
    const char *reason,
    uint32_t now) {
  out.print("[radio] diagnostic burst ");
  out.print(stateText);
  out.print(" elapsedMs=");
  out.print(diagnosticBurstElapsedMs(now));
  out.print(" attempts=");
  out.print(diagnosticBurstAttemptCount);
  out.print(" pass=");
  out.print(diagnosticBurstPassCount);
  out.print(" fail=");
  out.print(diagnosticBurstFailureCount);

  if (reason != nullptr && reason[0] != '\0') {
    out.print(" reason=\"");
    out.print(reason);
    out.print("\"");
  }

  out.println();
}

void clearDiagnosticBurstState() {
  diagnosticBurstRunning = false;
  diagnosticBurstStartedAtMs = 0;
  diagnosticBurstNextTxAtMs = 0;
}

} // namespace

namespace FWRadio {

const FwRadioHardware &hardware() {
  return kHeltecV4RadioHardware;
}

const FwRadioProfile *selectedProfile() {
  const FwRadioProfile *profile =
      fwRadioProfileById(fwSelectedRadioProfileId());
  return profile == nullptr ? fwDefaultRadioProfile() : profile;
}

const FwRadioProfile *appliedProfile() {
  if (!appliedProfileValid) {
    return nullptr;
  }

  return fwRadioProfileById(appliedProfileId);
}

const FwRadioChannel *selectedChannel() {
  const FwRadioChannel *channel =
      fwRadioChannelByNumber(fwSelectedRadioChannelNumber());
  return channel == nullptr ? fwDefaultRadioChannel() : channel;
}

const FwRadioChannel *appliedChannel() {
  if (!appliedChannelValid) {
    return nullptr;
  }

  return fwRadioChannelByNumber(appliedChannelNumber);
}

const char *selectedProfileKey() {
  const FwRadioProfile *profile = selectedProfile();
  return profile == nullptr ? "none" : profile->key;
}

const char *appliedProfileKey() {
  const FwRadioProfile *profile = appliedProfile();
  return profile == nullptr ? "none" : profile->key;
}

const char *selectedChannelKey() {
  const FwRadioChannel *channel = selectedChannel();
  return channel == nullptr ? "none" : channel->key;
}

const char *appliedChannelKey() {
  const FwRadioChannel *channel = appliedChannel();
  return channel == nullptr ? "none" : channel->key;
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

uint32_t txCount() {
  return radioTxCount;
}

int16_t lastTxResult() {
  return radioLastTxResult;
}

bool diagnosticBurstActive() {
  return diagnosticBurstRunning;
}

void printStatus(Stream &out) {
  const FwRadioProfile *profile = selectedProfile();
  const FwRadioChannel *channel = selectedChannel();

  out.print("[radio] state=");
  out.print(stateName());
  out.print(" result=");
  out.print(radioResult);
  out.print(" selectedProfile=");
  out.print(selectedProfileKey());
  out.print(" appliedProfile=");
  out.print(appliedProfileKey());
  out.print(" selectedChannel=");
  out.print(selectedChannelKey());
  out.print(" appliedChannel=");
  out.print(appliedChannelKey());

  if (profile == nullptr || channel == nullptr) {
    out.println();
    return;
  }

  out.print(" name=\"");
  out.print(profile->name);
  out.print("\" channelName=\"");
  out.print(channel->name);
  out.print("\" freqHz=");
  out.print(channel->frequencyHz);
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
  out.print(appliedPowerDbm(*profile));
  out.print(" txCount=");
  out.print(radioTxCount);
  out.print(" lastTxResult=");
  out.print(radioLastTxResult);
  out.print(" diagnosticBurst=");
  out.print(diagnosticBurstRunning ? "ACTIVE" : "IDLE");

  if (diagnosticBurstRunning) {
    out.print(" burstAttempts=");
    out.print(diagnosticBurstAttemptCount);
    out.print(" burstElapsedMs=");
    out.print(diagnosticBurstElapsedMs(millis()));
  }

  out.println();
}

bool beginDiagnostic(Stream &out) {
  const FwRadioProfile *profile = selectedProfile();
  const FwRadioChannel *channel = selectedChannel();
  if (profile == nullptr || channel == nullptr) {
    radioState = FwRadioState::Error;
    radioResult = RADIOLIB_ERR_UNKNOWN;
    out.println(
        "[radio] init failed: no selected/default profile or channel");
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
      static_cast<float>(channel->frequencyHz) / 1000000.0f;
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
    appliedProfileValid = false;
    appliedChannelValid = false;
    out.print("[radio] SX1262 init failed code=");
    out.println(radioResult);
    printStatus(out);
    return false;
  }

  radioResult = radio.setDio2AsRfSwitch(true);
  if (radioResult != RADIOLIB_ERR_NONE) {
    radioState = FwRadioState::Error;
    appliedProfileValid = false;
    appliedChannelValid = false;
    out.print("[radio] DIO2 RF switch setup failed code=");
    out.println(radioResult);
    printStatus(out);
    return false;
  }

  appliedProfileId = profile->id;
  appliedProfileValid = true;
  appliedChannelNumber = channel->number;
  appliedChannelValid = true;
  radioState = FwRadioState::Ready;

  out.println("[radio] SX1262 init PASS; no TX/RX started");
  printStatus(out);
  return true;
}

bool transmitDiagnostic(Stream &out) {
  const FwRadioProfile *profile = selectedProfile();
  const FwRadioChannel *channel = selectedChannel();
  if (profile == nullptr || channel == nullptr) {
    radioLastTxResult = RADIOLIB_ERR_UNKNOWN;
    out.println(
        "[radio] TX failed: no selected/default profile or channel");
    return false;
  }

  const FwRadioProfile *activeProfile = appliedProfile();
  const FwRadioChannel *activeChannel = appliedChannel();
  const bool profileChanged =
      activeProfile == nullptr || activeProfile->id != profile->id;
  const bool channelChanged =
      activeChannel == nullptr || activeChannel->number != channel->number;
  const bool configurationChanged = profileChanged || channelChanged;

  if (radioState != FwRadioState::Ready || configurationChanged) {
    if (configurationChanged && radioState == FwRadioState::Ready) {
      out.print("[radio] selected radio configuration changed: ");
      out.print("appliedProfile=");
      out.print(appliedProfileKey());
      out.print(" selectedProfile=");
      out.print(profile->key);
      out.print(" appliedChannel=");
      out.print(appliedChannelKey());
      out.print(" selectedChannel=");
      out.println(channel->key);
    } else {
      out.println("[radio] radio not ready; initializing first");
    }

    if (!beginDiagnostic(out)) {
      radioLastTxResult = radioResult;
      return false;
    }
  }

  FWPacket::Telemetry telemetry = {};
  telemetry.sequence = radioTxCount + 1;
  telemetry.distanceMm = FWPacket::kUnknownU16;
  telemetry.emptyMm = FWPacket::kUnknownU16;
  telemetry.fullMm = FWPacket::kUnknownU16;
  telemetry.fillPermille = FWPacket::kUnknownU16;
  telemetry.uptimeSeconds = millis() / 1000UL;
  FWPacket::readLocalSourceId(telemetry.sourceId);

  if (FWToF::hasLastValid()) {
    telemetry.flags |= FWPacket::kFlagTofValid;
    telemetry.distanceMm = FWToF::lastValidMm();
  }

  uint16_t stableAverageMm = 0;
  uint16_t stableSpanMm = 0;
  if (FWToF::stableReading(stableAverageMm, stableSpanMm)) {
    telemetry.flags |= FWPacket::kFlagTofStable;
    telemetry.distanceMm = stableAverageMm;
  }

  if (fwCalibrationConfigured()) {
    telemetry.flags |= FWPacket::kFlagCalibrated;
    telemetry.emptyMm = fwCalibrationEmptyMm();
    telemetry.fullMm = fwCalibrationFullMm();
    telemetry.fillPermille = FWPacket::calculateFillPermille(
        telemetry.distanceMm,
        telemetry.emptyMm,
        telemetry.fullMm);
  }

  uint8_t packet[FWPacket::kTelemetryPacketSize] = {};
  size_t packetSize = 0;
  if (!FWPacket::encodeTelemetry(
          telemetry, packet, sizeof(packet), packetSize)) {
    radioLastTxResult = RADIOLIB_ERR_UNKNOWN;
    out.println("[radio] TX failed: packet encode error");
    return false;
  }

  char sourceId[sizeof("FWP-000000")] = {};
  FWPacket::formatSourceId(
      telemetry.sourceId, sourceId, sizeof(sourceId));

  out.print("[radio] TX packet=v1.telemetry source=");
  out.print(sourceId);
  out.print(" sequence=");
  out.print(telemetry.sequence);
  out.print(" bytes=");
  out.print(packetSize);
  out.print(" distanceMm=");
  if (telemetry.distanceMm == FWPacket::kUnknownU16) {
    out.print("unknown");
  } else {
    out.print(telemetry.distanceMm);
  }
  out.print(" fillPermille=");
  if (telemetry.fillPermille == FWPacket::kUnknownU16) {
    out.println("unknown");
  } else {
    out.println(telemetry.fillPermille);
  }

  const uint32_t startedMs = millis();
  radioLastTxResult = radio.transmit(packet, packetSize);
  const uint32_t elapsedMs = millis() - startedMs;

  if (radioLastTxResult == RADIOLIB_ERR_NONE) {
    radioTxCount = telemetry.sequence;
    out.print("[radio] TX PASS elapsedMs=");
    out.println(elapsedMs);
  } else {
    out.print("[radio] TX failed code=");
    out.print(radioLastTxResult);
    out.print(" elapsedMs=");
    out.println(elapsedMs);
  }

  const int16_t standbyResult = radio.standby();
  out.print("[radio] standby result=");
  out.println(standbyResult);

  printStatus(out);
  return radioLastTxResult == RADIOLIB_ERR_NONE;
}

bool startDiagnosticBurst(Stream &out) {
  if (diagnosticBurstRunning) {
    out.print("[radio] diagnostic burst already active elapsedMs=");
    out.print(diagnosticBurstElapsedMs(millis()));
    out.print(" attempts=");
    out.println(diagnosticBurstAttemptCount);
    return false;
  }

  diagnosticBurstRunning = true;
  diagnosticBurstStartedAtMs = millis();
  diagnosticBurstNextTxAtMs = diagnosticBurstStartedAtMs;
  diagnosticBurstAttemptCount = 0;
  diagnosticBurstPassCount = 0;
  diagnosticBurstFailureCount = 0;

  out.print("[radio] diagnostic burst started durationMs=");
  out.print(kDiagnosticBurstDurationMs);
  out.print(" intervalMs=");
  out.print(kDiagnosticBurstIntervalMs);
  out.print(" expectedPackets=");
  out.println(kDiagnosticBurstExpectedPackets);
  return true;
}

void serviceDiagnosticBurst(Stream &out) {
  if (!diagnosticBurstRunning) {
    return;
  }

  const uint32_t now = millis();
  if (diagnosticBurstElapsedMs(now) >= kDiagnosticBurstDurationMs) {
    printDiagnosticBurstSummary(out, "complete", nullptr, now);
    clearDiagnosticBurstState();
    return;
  }

  if (static_cast<int32_t>(now - diagnosticBurstNextTxAtMs) < 0) {
    return;
  }

  diagnosticBurstAttemptCount++;

  out.print("[radio] diagnostic burst ping ");
  out.print(diagnosticBurstAttemptCount);
  out.print("/");
  out.println(kDiagnosticBurstExpectedPackets);

  if (transmitDiagnostic(out)) {
    diagnosticBurstPassCount++;
  } else {
    diagnosticBurstFailureCount++;
  }

  uint32_t nextTxAtMs =
      diagnosticBurstNextTxAtMs + kDiagnosticBurstIntervalMs;
  const uint32_t afterTxMs = millis();

  if (static_cast<int32_t>(afterTxMs - nextTxAtMs) >= 0) {
    nextTxAtMs = afterTxMs + kDiagnosticBurstIntervalMs;
  }

  diagnosticBurstNextTxAtMs = nextTxAtMs;
}

void cancelDiagnosticBurst(Stream &out, const char *reason) {
  if (!diagnosticBurstRunning) {
    return;
  }

  const uint32_t now = millis();
  printDiagnosticBurstSummary(out, "cancelled", reason, now);
  clearDiagnosticBurstState();
}

} // namespace FWRadio
