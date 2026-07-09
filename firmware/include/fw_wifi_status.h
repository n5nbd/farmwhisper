#pragma once

/*
 * Manual WiFi diagnostics for early FarmWhisper bring-up.
 *
 * This module is intentionally not a connection manager and not a captive
 * portal. At this stage WiFi must remain OFF unless a serial diagnostic
 * command explicitly asks for status or a one-shot scan.
 */


#include <Arduino.h>

namespace FWWiFiStatus {

void begin();
void printStatus(Stream &out);
void scanOnce(Stream &out);

} // namespace FWWiFiStatus
