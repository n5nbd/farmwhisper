#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

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
  constexpr uint16_t kNeoPixelCount = 1;
  constexpr uint8_t kNeoPixelBrightness = 16;

  Adafruit_NeoPixel pixel(kNeoPixelCount, kNeoPixelPin, NEO_GRB + NEO_KHZ800);

  unsigned long lastHeartbeatMs = 0;
  uint8_t currentNeoPixelIndex = 0;

  struct NeoPixelColor {
    const char* name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  const NeoPixelColor kNeoPixelColors[] = {
    {"red", 255, 0, 0},
    {"green", 0, 255, 0},
    {"blue", 0, 0, 255},
    {"off", 0, 0, 0},
  };

  void showNeoPixelColor() {
    const NeoPixelColor& color = kNeoPixelColors[currentNeoPixelIndex];
    pixel.setPixelColor(0, pixel.Color(color.red, color.green, color.blue));
    pixel.show();
  }

  void advanceNeoPixelSmokeTest() {
    currentNeoPixelIndex =
      (currentNeoPixelIndex + 1) % (sizeof(kNeoPixelColors) / sizeof(kNeoPixelColors[0]));
    showNeoPixelColor();
  }
}

void setup() {
  delay(500);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  pinMode(kButtonPin, INPUT_PULLUP);

  pixel.begin();
  pixel.setBrightness(kNeoPixelBrightness);
  pixel.clear();
  showNeoPixelColor();

  for (const char* line : kBootBanner) {
    Serial.println(line);
  }
}

void loop() {
  const unsigned long now = millis();
  if (now - lastHeartbeatMs >= 1000) {
    lastHeartbeatMs = now;

    const bool buttonPressed = (digitalRead(kButtonPin) == LOW);
    const NeoPixelColor& color = kNeoPixelColors[currentNeoPixelIndex];

    Serial.print("FarmWhisper heartbeat button=");
    Serial.print(buttonPressed ? "PRESSED" : "released");
    Serial.print(" pixel=");
    Serial.println(color.name);

    advanceNeoPixelSmokeTest();
  }
}