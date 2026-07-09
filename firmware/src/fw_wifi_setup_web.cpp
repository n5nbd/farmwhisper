#include "fw_wifi_setup_web.h"

namespace {

WebServer *setupServer = nullptr;
FWWiFiSetupWeb::StatusProvider provideStatus = nullptr;

constexpr const char *kSetupCss = R"CSS(
/*
 * FarmWhisper setup UI theme.
 *
 * Keep the .fw-* class names stable. Future themes should be able to replace
 * this stylesheet and any referenced assets without changing firmware logic or
 * the generated setup-page structure.
 */

:root {
  color-scheme: light;
}

* {
  box-sizing: border-box;
}

.fw-page {
  margin: 0;
  padding: 1rem;
  color: #000;
  background: #008080;
  font-family: Arial, Helvetica, sans-serif;
  line-height: 1.35;
}

.fw-window {
  max-width: 36rem;
  margin: 0 auto;
  color: #000;
  background: #c0c0c0;
  border-color: #fff #404040 #404040 #fff;
  border-style: solid;
  border-width: 2px;
  box-shadow: 1px 1px 0 #000;
}

.fw-titlebar {
  margin: 0;
  padding: 0.35rem 0.5rem;
  color: #fff;
  background: #000080;
  font-size: 1.15rem;
  font-weight: 700;
}

.fw-content {
  padding: 0.75rem;
}

.fw-intro,
.fw-note,
.fw-actions,
.fw-section {
  margin: 0.75rem 0 0;
}

.fw-intro {
  margin-top: 0;
}

.fw-status-grid {
  display: grid;
  grid-template-columns: 11rem 1fr;
  gap: 0.35rem 0.75rem;
  margin: 0.75rem 0 0;
  padding: 0.75rem;
  background: #fff;
  border-color: #404040 #fff #fff #404040;
  border-style: solid;
  border-width: 2px;
}

.fw-status-grid dt {
  font-weight: 700;
}

.fw-status-grid dd {
  margin: 0;
  overflow-wrap: anywhere;
  font-family: Consolas, "Courier New", monospace;
}

.fw-section {
  padding: 0.75rem;
  background: #c0c0c0;
  border-color: #404040 #fff #fff #404040;
  border-style: solid;
  border-width: 2px;
}

.fw-section-title {
  margin: 0;
  font-size: 1rem;
}

.fw-action-list {
  margin: 0.5rem 0 0;
  padding-left: 1.25rem;
}

.fw-action-list li + li {
  margin-top: 0.35rem;
}

.fw-link {
  color: #000080;
  font-weight: 700;
}
)CSS";

FWWiFiSetupWeb::SetupStatus currentStatus() {
  if (provideStatus != nullptr) {
    return provideStatus();
  }

  return {
      false,
      false,
      "FWP-000000",
      "FarmWhisper-000000",
      IPAddress(0, 0, 0, 0),
      0,
      0,
      0,
      0,
  };
}

void sendNoStore() {
  setupServer->sendHeader("Cache-Control", "no-store");
}

void handleSetupCss() {
  sendNoStore();
  setupServer->send(200, "text/css", kSetupCss);
}

String setupRootPageHtml(const FWWiFiSetupWeb::SetupStatus &status) {
  /*
   * Server-render the setup page for now. No JavaScript, no forms, and no
   * browser-side state are needed until the credential/config model exists.
   */
  String body;
  body.reserve(1800);

  body += "<!doctype html>\n";
  body += "<html lang=\"en\">\n";
  body += "<head>\n";
  body += "  <meta charset=\"utf-8\">\n";
  body += "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  body += "  <title>FarmWhisper Setup</title>\n";
  body += "  <link rel=\"stylesheet\" href=\"/setup.css\">\n";
  body += "</head>\n";
  body += "<body class=\"fw-page\">\n";
  body += "  <main class=\"fw-window\" aria-labelledby=\"fw-title\">\n";
  body += "    <h1 id=\"fw-title\" class=\"fw-titlebar\">FarmWhisper Setup</h1>\n";
  body += "    <section class=\"fw-content\">\n";
  body += "      <p class=\"fw-intro\">WiFi setup server is running.</p>\n";
  body += "      <dl class=\"fw-status-grid\">\n";

  body += "      <dt>AP smoke/setup</dt><dd>";
  body += status.apSmokeActive ? "ON" : "OFF";
  body += "</dd>\n";

  body += "      <dt>Setup HTTP</dt><dd>";
  body += status.setupHttpActive ? "ON" : "OFF";
  body += "</dd>\n";

  body += "      <dt>Device ID</dt><dd>";
  body += status.deviceId;
  body += "</dd>\n";

  body += "      <dt>SSID</dt><dd>";
  body += status.ssid;
  body += "</dd>\n";

  body += "      <dt>IP</dt><dd>";
  body += status.ip.toString();
  body += "</dd>\n";

  body += "      <dt>Stations</dt><dd>";
  body += static_cast<int>(status.stations);
  body += "</dd>\n";

  body += "      <dt>Age</dt><dd>";
  body += status.ageS;
  body += " s</dd>\n";

  body += "      <dt>Timeout</dt><dd>";
  body += status.timeoutS;
  body += " s</dd>\n";

  body += "      <dt>Remaining</dt><dd>";
  body += status.remainingS;
  body += " s</dd>\n";

  body += "      </dl>\n";
  body += "      <section class=\"fw-section\" aria-labelledby=\"fw-actions-title\">\n";
  body += "        <h2 id=\"fw-actions-title\" class=\"fw-section-title\">Actions</h2>\n";
  body += "        <ul class=\"fw-action-list\">\n";
  body += "          <li>WiFi credential entry: not implemented</li>\n";
  body += "          <li>Network scan from setup page: not implemented</li>\n";
  body += "          <li>Save/reboot: not implemented</li>\n";
  body += "        </ul>\n";
  body += "      </section>\n";
  body += "      <p class=\"fw-note\">Credential entry is not implemented in this slice.</p>\n";
  body += "      <p class=\"fw-actions\"><a class=\"fw-link\" href=\"/status\">View setup status JSON</a></p>\n";
  body += "    </section>\n";
  body += "  </main>\n";
  body += "</body>\n";
  body += "</html>\n";

  return body;
}

void handleSetupRoot() {
  const String body = setupRootPageHtml(currentStatus());

  sendNoStore();
  setupServer->send(200, "text/html", body);
}

void handleSetupStatus() {
  /*
   * Keep this dependency-free for now. A hand-built JSON response is enough
   * for route validation and avoids pulling in a JSON library before the setup
   * model exists.
   */
  const FWWiFiSetupWeb::SetupStatus status = currentStatus();

  String body;
  body.reserve(320);

  body += "{\n";
  body += "  \"apSmoke\": ";
  body += status.apSmokeActive ? "true" : "false";
  body += ",\n";
  body += "  \"setupHttp\": ";
  body += status.setupHttpActive ? "true" : "false";
  body += ",\n";
  body += "  \"deviceId\": \"";
  body += status.deviceId;
  body += "\",\n";
  body += "  \"ssid\": \"";
  body += status.ssid;
  body += "\",\n";
  body += "  \"ip\": \"";
  body += status.ip.toString();
  body += "\",\n";
  body += "  \"stations\": ";
  body += static_cast<int>(status.stations);
  body += ",\n";
  body += "  \"ageS\": ";
  body += status.ageS;
  body += ",\n";
  body += "  \"timeoutS\": ";
  body += status.timeoutS;
  body += ",\n";
  body += "  \"remainingS\": ";
  body += status.remainingS;
  body += "\n";
  body += "}\n";

  sendNoStore();
  setupServer->send(200, "application/json", body);
}

void handleSetupNotFound() {
  setupServer->send(404, "text/plain", "FarmWhisper setup placeholder: not found");
}

}  // namespace

namespace FWWiFiSetupWeb {

void registerRoutes(WebServer &server, StatusProvider statusProvider) {
  setupServer = &server;
  provideStatus = statusProvider;

  server.on("/", HTTP_GET, handleSetupRoot);
  server.on("/setup.css", HTTP_GET, handleSetupCss);
  server.on("/status", HTTP_GET, handleSetupStatus);
  server.onNotFound(handleSetupNotFound);
}

}  // namespace FWWiFiSetupWeb
