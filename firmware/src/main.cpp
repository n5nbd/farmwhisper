#include <Arduino.h>
#include "fw_config.h"
#include "fw_pins.h"

namespace {
  const char* kBootBanner[] = {
    "",
    "FarmWhisper firmware booting...",
    "Board: Heltec WiFi LoRa 32 V4 (ESP32-S3 class, assumed esp32-s3-devkitc-1 target)",
    "Display support is optional and disabled by default.",
  };

  unsigned long lastHeartbeatMs = 0;
}

void setup() {
  delay(500);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  for (const char* line : kBootBanner) {
    Serial.println(line);
  }
}

void loop() {
  const unsigned long now = millis();
  if (now - lastHeartbeatMs >= 1000) {
    lastHeartbeatMs = now;
    Serial.println("FarmWhisper heartbeat");
  }
}
