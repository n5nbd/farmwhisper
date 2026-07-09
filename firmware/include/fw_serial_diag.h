#pragma once

/*
 * Serial diagnostics module.
 *
 * Owns the interactive component-validation command surface. Commands should
 * be small, explicit, and regression-friendly. Adding a command must not alter
 * boot behavior or existing command behavior unless that is the purpose of the
 * slice being tested.
 */


namespace FWSerialDiag {

void printBootBanner();
void printHelp();
void handleCommands();
void printHeartbeat();
void printStatusSnapshot(const char *prefix);
void resetRuntimeDiagnostics();

}  // namespace FWSerialDiag
