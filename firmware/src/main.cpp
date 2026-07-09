#include <Arduino.h>

#include "fw_button.h"
#include "fw_config.h"
#include "fw_expansion_gpio.h"
#include "fw_pins.h"
#include "fw_product_i2c.h"
#include "fw_serial_diag.h"
#include "fw_status_pixel.h"
#include "fw_tof.h"
#include "fw_types.h"
#include "fw_wifi_status.h"

void setup() {
  delay(1200);

  Serial.begin(FWConfig::SerialBaud);
  delay(300);

  FWSerialDiag::printBootBanner();

  FWWiFiStatus::begin();
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
  FWSerialDiag::handleCommands();
  FWButton::update();

  if (FWButton::consumeTriplePressEvent()) {
    FWWiFiStatus::startApSetup(Serial);
  }

  FWToF::poll();
  FWStatusPixel::update(FWToF::status());
  FWSerialDiag::printHeartbeat();
}