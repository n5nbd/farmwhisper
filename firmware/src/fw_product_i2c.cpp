#include "fw_product_i2c.h"

#include <Wire.h>

#include "fw_pins.h"

namespace FWProductI2C {

void begin() {
  Wire.begin(FWPin::ProductI2cSda, FWPin::ProductI2cScl);
  Wire.setClock(400000);
}

bool scanFor(uint8_t expectedAddr) {
  Serial.println();
  Serial.print("[i2c] Scanning product I2C bus GPIO");
  Serial.print(FWPin::ProductI2cSda);
  Serial.print(" SDA / GPIO");
  Serial.print(FWPin::ProductI2cScl);
  Serial.println(" SCL");

  bool foundExpected = false;
  uint8_t foundCount = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    const uint8_t error = Wire.endTransmission();

    if (error == 0) {
      foundCount++;
      Serial.print("[i2c] Found device at 0x");
      if (addr < 16) {
        Serial.print('0');
      }
      Serial.println(addr, HEX);

      if (addr == expectedAddr) {
        foundExpected = true;
      }
    }
  }

  Serial.print("[i2c] Scan complete, devices=");
  Serial.print(foundCount);
  Serial.print(", expected 0x");
  if (expectedAddr < 16) {
    Serial.print('0');
  }
  Serial.print(expectedAddr, HEX);
  Serial.print(" present=");
  Serial.println(foundExpected ? "yes" : "no");

  return foundExpected;
}

}  // namespace FWProductI2C
