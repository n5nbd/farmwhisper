#include <Arduino.h>
#include <Wire.h>

#include "fw_button.h"
#include "fw_config.h"
#include "fw_expansion_gpio.h"
#include "fw_pins.h"
#include "fw_product_i2c.h"
#include "fw_serial_diag.h"
#include "fw_status_pixel.h"
#include "fw_tof.h"
#include "fw_types.h"

void setup() {
  delay(1200);

  Serial.begin(FWConfig::SerialBaud);
  delay(300);

  Serial.println();
  Serial.println("===== FarmWhisper component validation baseline =====");
  Serial.println("[boot] Heltec WiFi LoRa 32 V4 R2/R8");
  Serial.println("[boot] USB CDC serial enabled");
  Serial.println("[boot] Product I2C: SDA GPIO45, SCL GPIO46");
  Serial.println("[boot] Button: GPIO42 active LOW, raw IRQ + debounced app events");
  Serial.println("[boot] Button events: short press, long press, double press, triple press");
  Serial.println("[boot] NeoPixel: GPIO41 status model");
  Serial.println("[boot] GPIO37/38/39/40: spare/expansion GPIO smoke test as INPUT_PULLUP");
  Serial.println("[boot] Serial diagnostics: h/? help, s status, i i2c scan, g gpio smoke, v tof verbose, r reset counters");
  Serial.println("[boot] Display/OLED disabled");
  Serial.println("[boot] LoRa/WiFi/NVS/app calibration not enabled");

  FWStatusPixel::begin();

  FWButton::begin();
  FWExpansionGPIO::begin();

  Wire.begin(FWPin::ProductI2cSda, FWPin::ProductI2cScl);
  Wire.setClock(400000);

  const bool foundTof = FWProductI2C::scanFor(0x29);

  if (!FWToF::begin(foundTof)) {
    FWSerialDiag::printHelp();
    return;
  }

  Serial.println("[boot] Component validation loop started");

  FWSerialDiag::printHelp();
}

void loop() {
  FWSerialDiag::handleCommands();
  FWButton::update();
  FWToF::poll();
  FWStatusPixel::update(FWToF::status());
  FWSerialDiag::printHeartbeat();
}