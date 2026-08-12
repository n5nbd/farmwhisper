#if defined(FW_BOARD_XIAO_C6)

#include "fw_radio.h"

#include "fw_device_config.h"
#include "fw_ble.h"

namespace {

constexpr FwRadioHardware kNoRadioHardware = {-1, -1, -1, -1, -1, -1, -1};
constexpr int16_t kUnavailableResult = -1;
constexpr uint32_t kDiagnosticBurstDurationMs = 3UL * 60UL * 1000UL;
constexpr uint32_t kDiagnosticBurstIntervalMs = 5UL * 1000UL;
constexpr uint32_t kDiagnosticBurstExpectedPackets =
    kDiagnosticBurstDurationMs / kDiagnosticBurstIntervalMs;

bool diagnosticBurstRunning = false;
uint32_t diagnosticBurstStartedAtMs = 0;
uint32_t diagnosticBurstNextTxAtMs = 0;
uint32_t diagnosticBurstAttemptCount = 0;
uint32_t diagnosticBurstPassCount = 0;
uint32_t diagnosticBurstFailureCount = 0;

uint32_t diagnosticBurstElapsedMs(uint32_t now) {
  return diagnosticBurstRunning ? now - diagnosticBurstStartedAtMs : 0;
}

void clearDiagnosticBurstState() {
  diagnosticBurstRunning = false;
  diagnosticBurstStartedAtMs = 0;
  diagnosticBurstNextTxAtMs = 0;
}

void printDiagnosticBurstSummary(Stream &out, const char *stateText,
                                 const char *reason, uint32_t now) {
  out.print("[ble] diagnostic flood ");
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

}  // namespace

namespace FWRadio {

const FwRadioHardware &hardware() { return kNoRadioHardware; }

const FwRadioProfile *selectedProfile() {
  const FwRadioProfile *profile = fwRadioProfileById(fwSelectedRadioProfileId());
  return profile == nullptr ? fwDefaultRadioProfile() : profile;
}
const FwRadioProfile *appliedProfile() { return nullptr; }

const FwRadioChannel *selectedChannel() {
  const FwRadioChannel *channel = fwRadioChannelByNumber(fwSelectedRadioChannelNumber());
  return channel == nullptr ? fwDefaultRadioChannel() : channel;
}
const FwRadioChannel *appliedChannel() { return nullptr; }

const char *selectedProfileKey() {
  const FwRadioProfile *profile = selectedProfile();
  return profile == nullptr ? "none" : profile->key;
}
const char *appliedProfileKey() { return "none"; }
const char *selectedChannelKey() {
  const FwRadioChannel *channel = selectedChannel();
  return channel == nullptr ? "none" : channel->key;
}
const char *appliedChannelKey() { return "none"; }

FwRadioState state() { return FwRadioState::Disabled; }
const char *stateName() { return "DISABLED"; }
int16_t lastResult() { return kUnavailableResult; }
uint32_t txCount() { return 0; }
int16_t lastTxResult() { return kUnavailableResult; }
bool diagnosticBurstActive() { return diagnosticBurstRunning; }

void printStatus(Stream &out) {
  out.println("[radio] unavailable on XIAO ESP32-C6 hardware port");
}
bool beginDiagnostic(Stream &out) {
  out.println("[radio] LoRa unavailable on XIAO ESP32-C6 hardware port");
  return false;
}
bool transmitDiagnostic(Stream &out) {
  out.println("[radio] LoRa unavailable on XIAO ESP32-C6 hardware port");
  return false;
}
bool transmitTelemetry(const FWPacket::Telemetry &, Stream &out) {
  out.println("[radio] LoRa unavailable on XIAO ESP32-C6 hardware port");
  return false;
}
bool startDiagnosticBurst(Stream &out) {
  if (diagnosticBurstRunning) {
    out.print("[ble] diagnostic flood already active elapsedMs=");
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

  out.print("[ble] diagnostic flood started durationMs=");
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

  ++diagnosticBurstAttemptCount;
  out.print("[ble] diagnostic flood ping ");
  out.print(diagnosticBurstAttemptCount);
  out.print("/");
  out.println(kDiagnosticBurstExpectedPackets);

  if (FWBLE::transmitDiagnosticTelemetry(out)) {
    ++diagnosticBurstPassCount;
  } else {
    ++diagnosticBurstFailureCount;
  }

  uint32_t nextTxAtMs = diagnosticBurstNextTxAtMs + kDiagnosticBurstIntervalMs;
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
void serviceNormalBeacon(Stream &) {}
void resetNormalBeaconSchedule() {}

}  // namespace FWRadio

#endif
