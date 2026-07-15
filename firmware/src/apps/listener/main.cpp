#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

#include "fw_packet.h"
#include "fw_radio_channel.h"
#include "fw_radio_profile.h"

namespace {

constexpr uint32_t kSerialBaud = 115200;

constexpr int8_t kRadioNssPin = 8;
constexpr int8_t kRadioSckPin = 9;
constexpr int8_t kRadioMosiPin = 10;
constexpr int8_t kRadioMisoPin = 11;
constexpr int8_t kRadioResetPin = 12;
constexpr int8_t kRadioBusyPin = 13;
constexpr int8_t kRadioDio1Pin = 14;

constexpr int8_t kPacketLedPin = 35;
constexpr uint32_t kPacketLedPulseMs = 80;

constexpr float kTcxoVoltage = 1.8f;
constexpr uint8_t kPrivateSyncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
constexpr int8_t kRadioLibSx1262MaxPowerDbm = 22;

constexpr FwRadioProfileId kListenerProfileId =
    FwRadioProfileId::UsLongRange;
constexpr uint8_t kListenerChannelNumber = 9;

Module radioModule(
    kRadioNssPin,
    kRadioDio1Pin,
    kRadioResetPin,
    kRadioBusyPin);

SX1262 radio(&radioModule);

volatile bool packetReceived = false;
uint32_t packetCount = 0;
uint32_t receiveErrorCount = 0;
bool radioReady = false;
uint32_t lastHeartbeatMs = 0;
constexpr uint32_t kHeartbeatIntervalMs = 60000;

void onPacketReceived() {
  packetReceived = true;
}

const FwRadioProfile *listenerProfile() {
  const FwRadioProfile *profile =
      fwRadioProfileById(kListenerProfileId);
  return profile == nullptr ? fwDefaultRadioProfile() : profile;
}

const FwRadioChannel *listenerChannel() {
  const FwRadioChannel *channel =
      fwRadioChannelByNumber(kListenerChannelNumber);
  return channel == nullptr ? fwDefaultRadioChannel() : channel;
}

int8_t appliedPowerDbm(const FwRadioProfile &profile) {
  return profile.txPowerDbm > kRadioLibSx1262MaxPowerDbm
      ? kRadioLibSx1262MaxPowerDbm
      : profile.txPowerDbm;
}

void pulsePacketLed() {
  digitalWrite(kPacketLedPin, HIGH);
  delay(kPacketLedPulseMs);
  digitalWrite(kPacketLedPin, LOW);
}

void printProfile(
    const FwRadioProfile &profile,
    const FwRadioChannel &channel) {
  Serial.print("[listener] profile=");
  Serial.print(profile.key);
  Serial.print(" name=\"");
  Serial.print(profile.name);
  Serial.print("\" channel=");
  Serial.print(channel.key);
  Serial.print(" freqHz=");
  Serial.print(channel.frequencyHz);
  Serial.print(" bwHz=");
  Serial.print(profile.bandwidthHz);
  Serial.print(" sf=");
  Serial.print(profile.spreadingFactor);
  Serial.print(" cr=4/");
  Serial.print(profile.codingRateDenominator);
  Serial.print(" preamble=");
  Serial.println(profile.preambleSymbols);
}

bool beginRadio() {
  const FwRadioProfile *profile = listenerProfile();
  const FwRadioChannel *channel = listenerChannel();
  if (profile == nullptr || channel == nullptr) {
    Serial.println("[listener] ERROR no radio profile or channel");
    return false;
  }

  printProfile(*profile, *channel);

  SPI.begin(
      kRadioSckPin,
      kRadioMisoPin,
      kRadioMosiPin,
      kRadioNssPin);

  const float frequencyMhz =
      static_cast<float>(channel->frequencyHz) / 1000000.0f;
  const float bandwidthKhz =
      static_cast<float>(profile->bandwidthHz) / 1000.0f;

  int16_t state = radio.begin(
      frequencyMhz,
      bandwidthKhz,
      profile->spreadingFactor,
      profile->codingRateDenominator,
      kPrivateSyncWord,
      appliedPowerDbm(*profile),
      profile->preambleSymbols,
      kTcxoVoltage,
      false);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[listener] SX1262 init failed code=");
    Serial.println(state);
    return false;
  }

  state = radio.setDio2AsRfSwitch(true);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[listener] RF switch setup failed code=");
    Serial.println(state);
    return false;
  }

  radio.setPacketReceivedAction(onPacketReceived);

  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[listener] startReceive failed code=");
    Serial.println(state);
    return false;
  }

  radioReady = true;
  Serial.println("[listener] READY continuous receive");
  return true;
}

void printOptionalU16(uint16_t value) {
  if (value == FWPacket::kUnknownU16) {
    Serial.print("unknown");
  } else {
    Serial.print(value);
  }
}

void serviceReceivedPacket() {
  if (!packetReceived) {
    return;
  }

  packetReceived = false;

  const size_t packetLength = radio.getPacketLength();
  uint8_t payload[255] = {};
  const int16_t state = radio.readData(payload, packetLength);

  if (state == RADIOLIB_ERR_NONE) {
    packetCount++;

    FWPacket::Telemetry telemetry = {};
    if (FWPacket::decodeTelemetry(payload, packetLength, telemetry)) {
      char sourceId[sizeof("FWP-000000")] = {};
      FWPacket::formatSourceId(
          telemetry.sourceId, sourceId, sizeof(sourceId));

      Serial.print("[rx] count=");
      Serial.print(packetCount);
      Serial.print(" packet=v1.telemetry source=");
      Serial.print(sourceId);
      Serial.print(" sequence=");
      Serial.print(telemetry.sequence);
      Serial.print(" bytes=");
      Serial.print(packetLength);
      Serial.print(" distanceMm=");
      printOptionalU16(telemetry.distanceMm);
      Serial.print(" emptyMm=");
      printOptionalU16(telemetry.emptyMm);
      Serial.print(" fullMm=");
      printOptionalU16(telemetry.fullMm);
      Serial.print(" fillPermille=");
      printOptionalU16(telemetry.fillPermille);
      Serial.print(" uptimeS=");
      Serial.print(telemetry.uptimeSeconds);
      Serial.print(" tofValid=");
      Serial.print(
          (telemetry.flags & FWPacket::kFlagTofValid) != 0 ? "yes" : "no");
      Serial.print(" tofStable=");
      Serial.print(
          (telemetry.flags & FWPacket::kFlagTofStable) != 0 ? "yes" : "no");
      Serial.print(" calibrated=");
      Serial.print(
          (telemetry.flags & FWPacket::kFlagCalibrated) != 0 ? "yes" : "no");
      Serial.print(" rssi=");
      Serial.print(radio.getRSSI());
      Serial.print(" snr=");
      Serial.print(radio.getSNR());
      Serial.print(" freqErrorHz=");
      Serial.println(radio.getFrequencyError());
    } else {
      Serial.print("[rx] count=");
      Serial.print(packetCount);
      Serial.print(" packet=unrecognized bytes=");
      Serial.print(packetLength);
      Serial.print(" rssi=");
      Serial.print(radio.getRSSI());
      Serial.print(" snr=");
      Serial.println(radio.getSNR());
    }

    pulsePacketLed();
  } else {
    receiveErrorCount++;
    Serial.print("[rx] error=");
    Serial.print(state);
    Serial.print(" errorCount=");
    Serial.println(receiveErrorCount);
  }

  const int16_t restartState = radio.startReceive();
  if (restartState != RADIOLIB_ERR_NONE) {
    Serial.print("[listener] receive restart failed code=");
    Serial.println(restartState);
  }
}


void printHeartbeatIfDue() {
  const uint32_t now = millis();
  if ((now - lastHeartbeatMs) < kHeartbeatIntervalMs) {
    return;
  }

  lastHeartbeatMs = now;

  const FwRadioProfile *profile = listenerProfile();

  Serial.print("[listener] alive uptimeS=");
  Serial.print(now / 1000);
  Serial.print(" radio=");
  Serial.print(radioReady ? "READY" : "NOT_READY");
  Serial.print(" packets=");
  Serial.print(packetCount);
  Serial.print(" errors=");
  Serial.print(receiveErrorCount);
  Serial.print(" profile=");
  Serial.print(profile == nullptr ? "none" : profile->key);
  Serial.print(" channel=");
  const FwRadioChannel *channel = listenerChannel();
  Serial.println(channel == nullptr ? "none" : channel->key);
}

}  // namespace

void setup() {
  delay(1200);
  Serial.begin(kSerialBaud);
  delay(300);

  pinMode(kPacketLedPin, OUTPUT);
  digitalWrite(kPacketLedPin, LOW);

  Serial.println();
  Serial.println("===== FarmWhisper Heltec V4 Listener =====");
  Serial.println("[boot] GPIO35 pulses only on packet receipt");
  Serial.println("[boot] No WiFi, ToF, button, or NeoPixel logic");

  if (!beginRadio()) {
    Serial.println("[listener] HALTED");
  }
}

void loop() {
  serviceReceivedPacket();
  printHeartbeatIfDue();
  delay(1);
}
