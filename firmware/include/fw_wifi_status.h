#pragma once

#include <Arduino.h>

/*
 * Manual WiFi diagnostics for early FarmWhisper bring-up.
 *
 * This module is intentionally not a connection manager and not a captive
 * portal. At this stage WiFi must remain OFF unless a serial diagnostic
 * command explicitly asks for status, a one-shot scan, or a manual AP smoke
 * test. The AP smoke test owns a tiny placeholder HTTP server only while the
 * setup AP is active.
 */

namespace FWWiFiStatus {

/*
 * Initialize WiFi into the safest baseline state for component validation:
 * no persistence writes and radio OFF.
 */
void begin();

/*
 * Print current WiFi mode/status without changing radio state.
 */
void printStatus(Stream &out);

/*
 * Service manual WiFi diagnostics.
 *
 * This enforces AP smoke/setup timeout and polls the placeholder HTTP server
 * while AP smoke/setup mode is active. It must not start WiFi, scan, connect,
 * or run captive-portal behavior.
 */
void service(Stream &out);

/*
 * Run one synchronous scan, print results, delete scan data, then force WiFi
 * back OFF. This is a manual diagnostic only.
 */
void scanOnce(Stream &out);

/*
 * Toggle a manual SoftAP smoke test.
 *
 * This starts or stops only the ESP32 access point radio and its manual
 * placeholder HTTP server. It does not start a DNS server, captive portal,
 * credential UI, or storage layer.
 */
void toggleApSmoke(Stream &out);

} // namespace FWWiFiStatus
