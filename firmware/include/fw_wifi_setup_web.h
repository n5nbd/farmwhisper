#pragma once

/*
 * FarmWhisper WiFi setup web surface.
 *
 * Owns the manual setup HTTP routes and rendering for:
 *
 * - /
 * - /setup.css
 * - /status
 *
 * This module does not start/stop WiFi, own AP timeout policy, scan networks,
 * store credentials, or perform captive-portal behavior. The WiFi status
 * module owns setup/AP state and provides a read-only snapshot callback.
 */

#include <Arduino.h>
#include <IPAddress.h>
#include <WebServer.h>

namespace FWWiFiSetupWeb {

struct SetupStatus {
  bool apSmokeActive;
  bool setupHttpActive;
  const char *deviceId;
  const char *ssid;
  IPAddress ip;
  uint8_t stations;
  uint32_t ageS;
  uint32_t timeoutS;
  uint32_t remainingS;
};

using StatusProvider = SetupStatus (*)();

/*
 * Register setup web routes on the supplied server.
 *
 * The caller owns the WebServer object and its begin/close/handleClient
 * lifecycle. This module only attaches handlers and renders responses.
 */
void registerRoutes(WebServer &server, StatusProvider statusProvider);

/*
 * Clear the RAM-only setup unlock state.
 *
 * WiFi/AP lifecycle code calls this whenever the setup HTTP server/AP stops so
 * a future setup session starts locked again.
 */
void resetSession();

}  // namespace FWWiFiSetupWeb
