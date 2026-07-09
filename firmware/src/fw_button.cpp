#include "fw_button.h"

#include "fw_config.h"
#include "fw_pins.h"
#include "fw_status_pixel.h"
#include "fw_types.h"

namespace {

volatile uint32_t rawButtonIrqCount = 0;

bool lastRawButton = HIGH;
bool debouncedButton = HIGH;
uint32_t rawButtonChangedAtMs = 0;

uint32_t buttonPressCount = 0;
uint32_t buttonLongPressCount = 0;
uint32_t buttonDoublePressCount = 0;
uint32_t buttonTriplePressCount = 0;
bool triplePressEventPending = false;

uint32_t buttonPressedAtMs = 0;
bool buttonLongPressReported = false;

uint8_t buttonPendingShortPresses = 0;
uint32_t lastShortPressReleaseMs = 0;

void IRAM_ATTR onButtonFalling() {
  rawButtonIrqCount++;
}

void printButtonEventCounters() {
  Serial.print(" pressCount=");
  Serial.print(buttonPressCount);
  Serial.print(" longPressCount=");
  Serial.print(buttonLongPressCount);
  Serial.print(" doublePressCount=");
  Serial.print(buttonDoublePressCount);
  Serial.print(" triplePressCount=");
  Serial.print(buttonTriplePressCount);
  Serial.print(" pendingShortPresses=");
  Serial.print(buttonPendingShortPresses);
  Serial.print(" rawIrqCount=");
  Serial.print(rawButtonIrqCount);
}

void fireDoublePressEvent() {
  buttonDoublePressCount++;
  FWStatusPixel::triggerButtonOverlay(ButtonOverlay::DoublePress, FWConfig::ButtonMultiFlashMs);

  Serial.print("[button] doublePress");
  printButtonEventCounters();
  Serial.println();
}

void fireTriplePressEvent() {
  buttonTriplePressCount++;
  triplePressEventPending = true;
  FWStatusPixel::triggerButtonOverlay(ButtonOverlay::TriplePress, FWConfig::ButtonMultiFlashMs);

  Serial.print("[button] triplePress");
  printButtonEventCounters();
  Serial.println();
}

void finishPendingShortPressSequenceIfReady(uint32_t now) {
  if (debouncedButton == LOW) {
    return;
  }

  if (buttonPendingShortPresses == 0) {
    return;
  }

  if ((now - lastShortPressReleaseMs) < FWConfig::ButtonMultiPressGapMs) {
    return;
  }

  if (buttonPendingShortPresses == 2) {
    fireDoublePressEvent();
  } else if (buttonPendingShortPresses == 1) {
    Serial.print("[button] singleShortPressSequence");
    printButtonEventCounters();
    Serial.println();
  } else if (buttonPendingShortPresses >= 3) {
    fireTriplePressEvent();
  }

  buttonPendingShortPresses = 0;
}

}  // namespace

namespace FWButton {

void begin() {
  pinMode(FWPin::BigButton, INPUT_PULLUP);
  lastRawButton = digitalRead(FWPin::BigButton);
  debouncedButton = lastRawButton;
  rawButtonChangedAtMs = millis();

  attachInterrupt(digitalPinToInterrupt(FWPin::BigButton), onButtonFalling, FALLING);
}

const char *buttonText(bool value) {
  return value == LOW ? "LOW/pressed" : "HIGH/released";
}

void update() {
  const uint32_t now = millis();
  const bool rawButton = digitalRead(FWPin::BigButton);

  if (rawButton != lastRawButton) {
    lastRawButton = rawButton;
    rawButtonChangedAtMs = now;

    Serial.print("[button] raw=");
    Serial.print(buttonText(rawButton));
    Serial.print(" rawIrqCount=");
    Serial.println(rawButtonIrqCount);
  }

  if ((now - rawButtonChangedAtMs) >= FWConfig::ButtonDebounceMs && rawButton != debouncedButton) {
    const bool previousDebouncedButton = debouncedButton;
    debouncedButton = rawButton;

    Serial.print("[button] debounced=");
    Serial.print(buttonText(debouncedButton));

    if (debouncedButton == LOW) {
      buttonPressCount++;
      buttonPressedAtMs = now;
      buttonLongPressReported = false;
      FWStatusPixel::triggerButtonOverlay(ButtonOverlay::ShortPress, FWConfig::ButtonShortFlashMs);

      printButtonEventCounters();
    } else if (previousDebouncedButton == LOW) {
      const uint32_t heldMs = now - buttonPressedAtMs;

      Serial.print(" heldMs=");
      Serial.print(heldMs);
      Serial.print(" longPressSeen=");
      Serial.print(buttonLongPressReported ? "yes" : "no");

      if (!buttonLongPressReported) {
        buttonPendingShortPresses++;
        lastShortPressReleaseMs = now;

        if (buttonPendingShortPresses >= 3) {
          fireTriplePressEvent();
          buttonPendingShortPresses = 0;
        } else {
          printButtonEventCounters();
        }
      } else {
        buttonPendingShortPresses = 0;
        printButtonEventCounters();
      }
    }

    Serial.println();
  }

  if (debouncedButton == LOW && !buttonLongPressReported) {
    const uint32_t heldMs = now - buttonPressedAtMs;

    if (heldMs >= FWConfig::ButtonLongPressMs) {
      buttonLongPressReported = true;
      buttonLongPressCount++;
      buttonPendingShortPresses = 0;
      FWStatusPixel::triggerButtonOverlay(ButtonOverlay::LongPress, FWConfig::ButtonLongFlashMs);

      Serial.print("[button] longPress heldMs=");
      Serial.print(heldMs);
      printButtonEventCounters();
      Serial.println();
    }
  }

  finishPendingShortPressSequenceIfReady(now);
}

void resetDiagnostics() {
  noInterrupts();
  rawButtonIrqCount = 0;
  interrupts();

  buttonPressCount = 0;
  buttonLongPressCount = 0;
  buttonDoublePressCount = 0;
  buttonTriplePressCount = 0;
  triplePressEventPending = false;

  buttonPendingShortPresses = 0;
  lastShortPressReleaseMs = 0;
  buttonLongPressReported = false;
}

uint32_t rawIrqCount() {
  noInterrupts();
  const uint32_t value = rawButtonIrqCount;
  interrupts();
  return value;
}

uint32_t pressCount() {
  return buttonPressCount;
}

uint32_t longPressCount() {
  return buttonLongPressCount;
}

uint32_t doublePressCount() {
  return buttonDoublePressCount;
}

uint32_t triplePressCount() {
  return buttonTriplePressCount;
}

uint8_t pendingShortPresses() {
  return buttonPendingShortPresses;
}

bool consumeTriplePressEvent() {
  if (!triplePressEventPending) {
    return false;
  }

  triplePressEventPending = false;
  return true;
}

}  // namespace FWButton
