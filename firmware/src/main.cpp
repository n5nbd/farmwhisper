#include <Arduino.h>

#if defined(FW_BOARD_XIAO_C6)
#include <Wire.h>
#include <esp_sleep.h>
#endif

#include "fw_ble.h"
#include "fw_battery.h"
#include "fw_button.h"
#include "fw_config.h"
#include "fw_device_config.h"
#include "fw_expansion_gpio.h"
#include "fw_pins.h"
#include "fw_product_i2c.h"
#include "fw_radio.h"
#include "fw_serial_diag.h"
#include "fw_status_pixel.h"
#include "fw_tof.h"
#include "fw_transport_mode.h"
#include "fw_types.h"
#include "fw_wifi_status.h"

#if defined(FW_BOARD_XIAO_C6)
namespace {

constexpr uint32_t kC6ButtonServiceWindowMs = 30000UL;
constexpr uint32_t kC6NormalCycleMaxAwakeMs = 2500UL;

esp_sleep_wakeup_cause_t c6WakeCause = ESP_SLEEP_WAKEUP_UNDEFINED;
uint32_t c6CycleStartedAtMs = 0;
uint32_t c6StayAwakeUntilMs = 0;
bool c6TimerTelemetryHandled = false;

bool c6TimeBefore(uint32_t deadlineMs) {
  return deadlineMs != 0 &&
         static_cast<int32_t>(millis() - deadlineMs) < 0;
}

void c6ExtendServiceWindow() {
  c6StayAwakeUntilMs = millis() + kC6ButtonServiceWindowMs;
}

void c6EnterDeepSleep() {
  const uint8_t beaconsPerHour = fwBeaconsPerHour();
  const uint32_t sleepMs =
      3600000UL / static_cast<uint32_t>(beaconsPerHour);

  FWBLE::prepareForDeepSleep(sleepMs);

  // Proven C6 rail shutdown sequence: remove drive from the powered-off I2C
  // devices and NeoPixel before opening the active-low switched 3.3 V rail.
  Wire.end();
  pinMode(FWPin::ProductI2cSda, INPUT);
  pinMode(FWPin::ProductI2cScl, INPUT);
  pinMode(FWPin::StatusPixel, OUTPUT);
  digitalWrite(FWPin::StatusPixel, LOW);
  digitalWrite(FWPin::SensorRailEnable, HIGH);

  pinMode(FWPin::BigButton, INPUT_PULLUP);
  const uint32_t releaseDeadline = millis() + 750UL;
  while (digitalRead(FWPin::BigButton) == LOW &&
         static_cast<int32_t>(millis() - releaseDeadline) < 0) {
    delay(10);
  }

  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(sleepMs) * 1000ULL);
  esp_deep_sleep_enable_gpio_wakeup(
      1ULL << FWPin::BigButton, ESP_GPIO_WAKEUP_GPIO_LOW);

  Serial.print("[power] sleepMs=");
  Serial.print(sleepMs);
  Serial.print(" rail=");
  Serial.print(FWPin::SensorRailEnable);
  Serial.println(" HIGH/off buttonWake=LOW");
  Serial.flush();
  delay(10);
  esp_deep_sleep_start();
}

bool c6NormalCycleComplete() {
  if (FWToF::validCount() >= 5) {
    return true;
  }
  return (millis() - c6CycleStartedAtMs) >= kC6NormalCycleMaxAwakeMs;
}

}  // namespace
#endif

void setup() {
#if defined(FW_BOARD_XIAO_C6)
  pinMode(FWPin::SensorRailEnable, OUTPUT);
  digitalWrite(FWPin::SensorRailEnable, LOW);
  c6WakeCause = esp_sleep_get_wakeup_cause();
#endif

  delay(1200);
  Serial.begin(FWConfig::SerialBaud);
  delay(300);

  FWSerialDiag::printBootBanner();
#if defined(FW_BOARD_XIAO_C6)
  Serial.print("[power] wakeCause=");
  Serial.println(static_cast<int>(c6WakeCause));
#endif

  FWWiFiStatus::begin(Serial);
  FWBLE::begin(Serial);
  FWStatusPixel::begin();
  FWButton::begin();
  FWBattery::begin();
  FWExpansionGPIO::begin();
  FWProductI2C::begin();

  const bool foundTof = FWProductI2C::scanFor(0x29);
  if (!FWToF::begin(foundTof)) {
    FWSerialDiag::printHelp();
    return;
  }

  Serial.println("[boot] Component validation loop started");
  FWSerialDiag::printHelp();
#if defined(FW_BOARD_XIAO_C6)
  c6CycleStartedAtMs = millis();
  if (c6WakeCause != ESP_SLEEP_WAKEUP_TIMER) {
    c6ExtendServiceWindow();
    Serial.print("[power] service windowMs=");
    Serial.print(kC6ButtonServiceWindowMs);
    Serial.print(" wakeCause=");
    Serial.println(static_cast<int>(c6WakeCause));
  }
#endif
}

void loop() {
  FWWiFiStatus::service(Serial);
  FWBLE::service(Serial);
  FWSerialDiag::handleCommands();
  FWButton::update();

  if (FWButton::consumeRecoveryHoldEvent()) {
    if (FWWiFiStatus::clearSetupPinWithRecovery(Serial)) {
      FWStatusPixel::flashRed(5, 150, 150);
    }
  }

  if (FWButton::consumeDoublePressEvent()) {
    fwClearCalibration();
    Serial.println(
        "[calibration] saved calibration cleared by button double press");
  }

  if (FWButton::consumeTriplePressEvent()) {
#if defined(FW_BOARD_XIAO_C6)
    // Give the existing AP startup path a no-sleep bridge so the power manager
    // cannot race the triple-press maintenance request.
    c6ExtendServiceWindow();
#endif
    FWWiFiStatus::startApSetup(Serial);
  }

#if defined(FW_BOARD_XIAO_C6)
  FWRadio::serviceDiagnosticBurst(Serial);
#else
  const FwTransportModeId transportMode = fwSelectedTransportModeId();
  if (fwTransportModeUsesLoRa(transportMode)) {
    FWRadio::serviceDiagnosticBurst(Serial);
    FWRadio::serviceNormalBeacon(Serial);
  } else if (FWRadio::diagnosticBurstActive()) {
    FWRadio::cancelDiagnosticBurst(
        Serial, "selected transport mode no longer includes LoRa");
    FWRadio::resetNormalBeaconSchedule();
  } else {
    FWRadio::resetNormalBeaconSchedule();
  }
#endif

  FWToF::poll();
  FWStatusPixel::update(FWToF::status());
  FWSerialDiag::printHeartbeat();

#if defined(FW_BOARD_XIAO_C6)
  if (FWWiFiStatus::setupApActive() ||
      FWRadio::diagnosticBurstActive() ||
      FWBLE::normalTelemetryHoldActive() ||
      c6TimeBefore(c6StayAwakeUntilMs)) {
    return;
  }

  if (c6NormalCycleComplete()) {
    // A timer wake means the configured telemetry interval has elapsed. Send
    // exactly one normal FarmWhisper BLE telemetry update before sleeping
    // again. The existing scheduler may already have sent it during this boot;
    // its one-second hold tells us that happened, so do not duplicate it.
    if (c6WakeCause == ESP_SLEEP_WAKEUP_TIMER && !c6TimerTelemetryHandled) {
      if (!FWBLE::normalTelemetryHoldActive()) {
        FWBLE::transmitScheduledTelemetry(Serial);
      }
      c6TimerTelemetryHandled = true;
      return;
    }

    c6EnterDeepSleep();
  }
#endif
}
