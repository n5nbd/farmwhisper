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
 * Initialize FarmWhisper WiFi setup state and start the setup AP.
 *
 * The setup AP timeout begins at startup. WiFi persistence remains disabled.
 */
void begin(Stream &out);

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

/* Return true while the setup AP/HTTP session is active. */
bool setupApActive();

/*
 * Run one synchronous scan, print results, delete scan data, then force WiFi
 * back OFF. This is a manual diagnostic only.
 */
void scanOnce(Stream &out);

/*
 * Start manual setup AP/HTTP mode, or refresh its timeout if already active.
 *
 * This is intended for the physical UI path, such as a triple button press.
 * Serial command `a` remains a bench diagnostic toggle.
 */
void startApSetup(Stream &out);

/*
 * Refresh the active setup AP timeout.
 *
 * This remains available to direct non-web callers. Setup web form submissions
 * report activity through the setup web callback.
 */
void refreshApSetupTimeout(Stream &out);

/*
 * Toggle a manual SoftAP smoke test.
 *
 * This starts or stops only the ESP32 access point radio and its manual
 * placeholder HTTP server. It does not start a DNS server, captive portal,
 * credential UI, or storage layer.
 */
void toggleApSmoke(Stream &out);

/*
 * Clear the stored local setup PIN.
 *
 * This only succeeds while the setup AP is active. It is intended for
 * the deliberate physical recovery gesture.
 */
bool clearSetupPinWithRecovery(Stream &out);

} // namespace FWWiFiStatus
