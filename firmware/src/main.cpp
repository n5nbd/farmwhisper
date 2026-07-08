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

  constexpr int kButtonPin = FW_BIG_BUTTON_CANDIDATE;
  constexpr int kNeoPixelPin = FW_NEOPIXEL_CANDIDATE;
  unsigned long lastHeartbeatMs = 0;

  void writeNeoPixelByte(uint8_t value) {
    for (uint8_t bit = 0; bit < 8; ++bit) {
      digitalWrite(kNeoPixelPin, HIGH);
      delayMicroseconds(4);
      digitalWrite(kNeoPixelPin, (value & 0x80) ? HIGH : LOW);
      delayMicroseconds(4);
      digitalWrite(kNeoPixelPin, LOW);
      delayMicroseconds(4);
      value <<= 1;
    }
  }

  void writeNeoPixelColor(uint8_t red, uint8_t green, uint8_t blue) {
    writeNeoPixelByte(green);
    writeNeoPixelByte(red);
    writeNeoPixelByte(blue);
  }

  void runNeoPixelSmokeTest() {
    pinMode(kNeoPixelPin, OUTPUT);
    digitalWrite(kNeoPixelPin, LOW);
    writeNeoPixelColor(2, 2, 2);
    delayMicroseconds(50);
  }
}

void setup() {
  delay(500);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  pinMode(kButtonPin, INPUT_PULLUP);
  runNeoPixelSmokeTest();

  for (const char* line : kBootBanner) {
    Serial.println(line);
  }
}

void loop() {
  const unsigned long now = millis();
  if (now - lastHeartbeatMs >= 1000) {
    lastHeartbeatMs = now;
    Serial.println("FarmWhisper heartbeat");
    const bool buttonPressed = (digitalRead(kButtonPin) == LOW);
    Serial.println(buttonPressed ? "Button: PRESSED" : "Button: released");
  }
}
