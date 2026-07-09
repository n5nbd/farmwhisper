#pragma once

/*
 * Button module.
 *
 * Owns the validated active-LOW GPIO42 user button path, including debounce
 * and short/long/double/triple press classification. Preserve existing event
 * behavior unless intentionally re-validating the human-interface contract.
 */


#include <Arduino.h>

namespace FWButton {

void begin();
void update();
void resetDiagnostics();

const char *buttonText(bool value);

uint32_t rawIrqCount();
uint32_t pressCount();
uint32_t longPressCount();
uint32_t doublePressCount();
uint32_t triplePressCount();
uint8_t pendingShortPresses();

}  // namespace FWButton
