#if defined(FW_BOARD_XIAO_C6)

#include "fw_expansion_gpio.h"

#include <Arduino.h>

namespace FWExpansionGPIO {

void begin() {}

void printSmokeStatus(const char *prefix) {
  Serial.print(prefix);
  Serial.println(" unavailable on XIAO ESP32-C6 hardware port");
}

}  // namespace FWExpansionGPIO

#endif
