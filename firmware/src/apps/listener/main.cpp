#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

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

Module radioModule(
    kRadioNssPin,
    kRadioDio1Pin,
    kRadioResetPin,
    kRadioBusyPin);

SX1262 radio(&radioModule);

volatile bool packetReceived = false;
uint32_t packetCount = 0;
uint32_t receiveErrorCount = 0;

void onPacketReceived() {
  packetReceived = true;
}

const FwRadioProfile *listenerProfile() {
  const FwRadioProfile *profile =
      fwRadioProfileById(kListenerProfileId);
  return profile == nullptr ? fwDefaultRadioProfile() : profile;
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

void printProfile(const FwRadioProfile &profile) {
  Serial.print("[listener] profile=");
  Serial.print(profile.key);
  Serial.print(" name=\"");
  Serial.print(profile.name);
  Serial.print("\" freqHz=");
  Serial.print(profile.frequencyHz);
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
  if (profile == nullptr) {
    Serial.println("[listener] ERROR no radio profile");
    return false;
  }

  printProfile(*profile);

  SPI.begin(
      kRadioSckPin,
      kRadioMisoPin,
      kRadioMosiPin,
      kRadioNssPin);

  const float frequencyMhz =
      static_cast<float>(profile->frequencyHz) / 1000000.0f;
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

  Serial.println("[listener] READY continuous receive");
  return true;
}

void serviceReceivedPacket() {
  if (!packetReceived) {
    return;
  }

  packetReceived = false;

  String payload;
  const int16_t state = radio.readData(payload);

  if (state == RADIOLIB_ERR_NONE) {
    packetCount++;

    Serial.print("[rx] count=");
    Serial.print(packetCount);
    Serial.print(" bytes=");
    Serial.print(payload.length());
    Serial.print(" rssi=");
    Serial.print(radio.getRSSI());
    Serial.print(" snr=");
    Serial.print(radio.getSNR());
    Serial.print(" freqErrorHz=");
    Serial.print(radio.getFrequencyError());
    Serial.print(" payload=\"");
    Serial.print(payload);
    Serial.println("\"");

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
  delay(1);
}
