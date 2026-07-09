#pragma once

/*
 * Expansion GPIO smoke-test module.
 *
 * Owns the GPIO37/38/39/40 INPUT_PULLUP diagnostics. These pins are treated as
 * simple spare inputs during the validated baseline; future expansion features
 * should preserve an easy smoke-test path for board bring-up.
 */


namespace FWExpansionGPIO {

void begin();
void printSmokeStatus(const char *prefix);

}  // namespace FWExpansionGPIO
