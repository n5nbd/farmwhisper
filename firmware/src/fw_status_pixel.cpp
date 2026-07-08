#include "fw_status_pixel.h"

#include <Adafruit_NeoPixel.h>

#include "fw_pins.h"
#include "fw_types.h"

namespace {

Adafruit_NeoPixel pixel(1, FWPin::StatusPixel, NEO_GRB + NEO_KHZ800);

uint32_t buttonFlashUntilMs = 0;
ButtonOverlay buttonOverlay = ButtonOverlay::None;

void setPixel(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

}  // namespace

namespace FWStatusPixel {

void begin() {
  pixel.begin();
  pixel.setBrightness(40);
  setPixel(0, 0, 30);
}

void triggerButtonOverlay(ButtonOverlay overlay, uint32_t durationMs) {
  buttonOverlay = overlay;
  buttonFlashUntilMs = millis() + durationMs;
}

void clearButtonOverlay() {
  buttonOverlay = ButtonOverlay::None;
  buttonFlashUntilMs = 0;
}

void update(ComponentStatus componentStatus) {
  const uint32_t now = millis();

  if (now < buttonFlashUntilMs) {
    switch (buttonOverlay) {
      case ButtonOverlay::ShortPress:
        setPixel(40, 40, 40);  // white
        return;

      case ButtonOverlay::LongPress:
        setPixel(0, 40, 40);   // cyan
        return;

      case ButtonOverlay::DoublePress:
        setPixel(0, 0, 45);    // blue
        return;

      case ButtonOverlay::TriplePress:
        setPixel(45, 0, 45);   // magenta
        return;

      case ButtonOverlay::None:
        break;
    }
  } else {
    buttonOverlay = ButtonOverlay::None;
  }

  const bool blinkFast = ((now / 125) % 2) == 0;
  const bool blinkSlow = ((now / 500) % 2) == 0;

  switch (componentStatus) {
    case ComponentStatus::Booting:
      setPixel(0, 0, 30);
      break;

    case ComponentStatus::TofInitFailed:
      setPixel(blinkFast ? 45 : 0, 0, 0);
      break;

    case ComponentStatus::TofTimeout:
      setPixel(blinkSlow ? 45 : 0, 0, 0);
      break;

    case ComponentStatus::TofShady:
      setPixel(blinkFast ? 35 : 0, 0, blinkFast ? 35 : 0);
      break;

    case ComponentStatus::TofWarming:
      setPixel(0, 0, blinkSlow ? 35 : 8);
      break;

    case ComponentStatus::TofUnstable:
      setPixel(blinkSlow ? 35 : 6, blinkSlow ? 20 : 3, 0);
      break;

    case ComponentStatus::TofStable:
      setPixel(0, 35, 0);
      break;
  }
}

}  // namespace FWStatusPixel
