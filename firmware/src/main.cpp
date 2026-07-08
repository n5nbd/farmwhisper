#include <Arduino.h>
#include "fw_config.h"
#include "fw_pins.h"

void setup() {
  delay(500);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println();
  Serial.println("FarmWhisper firmware booting...");
  Serial.println("Board: Heltec WiFi LoRa 32 V4 (ESP32-S3 class, assumed esp32-s3-devkitc-1 target)");
  Serial.println("Display support is optional and disabled by default.");
}

void loop() {
  delay(1000);
}
