#include <Arduino.h>

#include "fw_ble.h"
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

void setup() {
  delay(1200);
  Serial.begin(FWConfig::SerialBaud);
  delay(300);

  FWSerialDiag::printBootBanner();

  FWWiFiStatus::begin(Serial);
  FWBLE::begin(Serial);
  FWStatusPixel::begin();
  FWButton::begin();
  FWExpansionGPIO::begin();
  FWProductI2C::begin();

  const bool foundTof = FWProductI2C::scanFor(0x29);
  if (!FWToF::begin(foundTof)) {
    FWSerialDiag::printHelp();
    return;
  }

  Serial.println("[boot] Component validation loop started");
  FWSerialDiag::printHelp();
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
    const FwTransportModeId transportMode = fwSelectedTransportModeId();

    if (!fwTransportModeUsesLoRa(transportMode)) {
      Serial.print(
          "[transport] LoRa diagnostic burst skipped: selected mode ");
      Serial.print(fwTransportModeName(transportMode));
      Serial.println(" disables LoRa");
    } else if (FWRadio::diagnosticBurstActive()) {
      FWRadio::cancelDiagnosticBurst(
          Serial, "button double press");
    } else {
      FWRadio::startDiagnosticBurst(Serial);
    }
  }

  if (FWButton::consumeTriplePressEvent()) {
    FWWiFiStatus::startApSetup(Serial);
  }

  const FwTransportModeId transportMode = fwSelectedTransportModeId();
  if (fwTransportModeUsesLoRa(transportMode)) {
    FWRadio::serviceDiagnosticBurst(Serial);
  } else if (FWRadio::diagnosticBurstActive()) {
    FWRadio::cancelDiagnosticBurst(
        Serial, "selected transport mode no longer includes LoRa");
  }

  FWToF::poll();
  FWStatusPixel::update(FWToF::status());
  FWSerialDiag::printHeartbeat();
}
