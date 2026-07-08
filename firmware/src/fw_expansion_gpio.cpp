#include "fw_expansion_gpio.h"

#include <Arduino.h>

#include "fw_pins.h"

namespace {

const char *groundStateText(uint8_t pin) {
  return digitalRead(pin) == LOW ? "LOW/grounded" : "HIGH/open";
}

}  // namespace

namespace FWExpansionGPIO {

void begin() {
  pinMode(FWPin::SpareGpio37, INPUT_PULLUP);
  pinMode(FWPin::ExpansionGpio38, INPUT_PULLUP);
  pinMode(FWPin::ExpansionGpio39, INPUT_PULLUP);
  pinMode(FWPin::ExpansionGpio40, INPUT_PULLUP);
}

void printSmokeStatus(const char *prefix) {
  Serial.print(prefix);
  Serial.print(" gpio37=");
  Serial.print(groundStateText(FWPin::SpareGpio37));
  Serial.print(" gpio38=");
  Serial.print(groundStateText(FWPin::ExpansionGpio38));
  Serial.print(" gpio39=");
  Serial.print(groundStateText(FWPin::ExpansionGpio39));
  Serial.print(" gpio40=");
  Serial.print(groundStateText(FWPin::ExpansionGpio40));
  Serial.println(" mode=INPUT_PULLUP expected=HIGH/open LOW/jumpered-to-GND");
}

}  // namespace FWExpansionGPIO
