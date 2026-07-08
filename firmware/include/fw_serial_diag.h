#pragma once

namespace FWSerialDiag {

void printBootBanner();
void printHelp();
void handleCommands();
void printHeartbeat();
void printStatusSnapshot(const char *prefix);
void resetRuntimeDiagnostics();

}  // namespace FWSerialDiag
