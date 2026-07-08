#pragma once

namespace FWSerialDiag {

void printHelp();
void handleCommands();
void printHeartbeat();
void printStatusSnapshot(const char *prefix);
void resetRuntimeDiagnostics();

}  // namespace FWSerialDiag
