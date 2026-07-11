#include "fw_wifi_setup_web.h"

#include "fw_device_config.h"
#include "fw_radio.h"
#include "fw_radio_profile.h"
#include "fw_transport_mode.h"

#include <Preferences.h>
#include <cstring>

namespace {

void appendHtmlEscapedString(String &out, const char *value) {
  if (value == nullptr) {
    return;
  }

  for (const char *p = value; *p != '\0'; ++p) {
    switch (*p) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&#39;";
        break;
      default:
        out += *p;
        break;
    }
  }
}

void appendJsonEscapedString(String &out, const char *value) {
  if (value == nullptr) {
    return;
  }

  for (const char *p = value; *p != '\0'; ++p) {
    switch (*p) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(*p) < 0x20) {
          out += ' ';
        } else {
          out += *p;
        }
        break;
    }
  }
}

void appendRadioProfileOptions(String &out) {
  const FwRadioProfileId selectedId = fwSelectedRadioProfileId();

  for (size_t i = 0; i < fwRadioProfileCount(); ++i) {
    const FwRadioProfile *profile = fwRadioProfileAt(i);
    if (profile == nullptr) {
      continue;
    }

    out += R"HTML(<option value=")HTML";
    appendHtmlEscapedString(out, profile->key);
    out += "\"";
    if (profile->id == selectedId) {
      out += " selected";
    }
    out += ">";
    appendHtmlEscapedString(out, profile->name);
    out += "</option>";
  }
}

void appendTransportModeOptions(String &out) {
  const FwTransportModeId selectedId = fwSelectedTransportModeId();

  for (size_t i = 0; i < fwTransportModeCount(); ++i) {
    const FwTransportMode *mode = fwTransportModeAt(i);
    if (mode == nullptr) {
      continue;
    }

    out += R"HTML(<option value=")HTML";
    appendHtmlEscapedString(out, mode->key);
    out += "\"";
    if (mode->id == selectedId) {
      out += " selected";
    }
    out += ">";
    appendHtmlEscapedString(out, mode->name);
    out += "</option>";
  }
}

WebServer *setupServer = nullptr;
FWWiFiSetupWeb::StatusProvider provideStatus = nullptr;

constexpr uint8_t kSetupPinDigits = 6;
constexpr const char *kSetupPrefsNamespace = "fw_setup";
constexpr const char *kSetupPinKey = "pin";

char setupPin[kSetupPinDigits + 1] = "";
bool setupPinConfigured = false;
bool setupUnlocked = false;
String setupNotice;
bool setupNoticeIsError = false;

void setSetupNotice(const String &message, bool isError = false) {
  setupNotice = message;
  setupNoticeIsError = isError;
}

void clearSetupNotice() {
  setupNotice = "";
  setupNoticeIsError = false;
}

bool isSixDigitPin(const String &pin) {
  if (pin.length() != kSetupPinDigits) {
    return false;
  }

  for (uint8_t i = 0; i < kSetupPinDigits; ++i) {
    if (!isDigit(pin[i])) {
      return false;
    }
  }

  return true;
}

void clearSetupPinFromRam() {
  setupPin[0] = '\0';
  setupPinConfigured = false;
  setupUnlocked = false;
}

bool saveSetupPinToNvs(const char *pin) {
  Preferences prefs;
  if (!prefs.begin(kSetupPrefsNamespace, false)) {
    return false;
  }

  const size_t written = prefs.putString(kSetupPinKey, pin);
  prefs.end();
  return written > 0;
}

bool clearSetupPinFromNvs() {
  Preferences prefs;
  if (!prefs.begin(kSetupPrefsNamespace, false)) {
    return false;
  }

  prefs.remove(kSetupPinKey);
  prefs.end();
  return true;
}

void loadSetupPinFromNvs() {
  clearSetupPinFromRam();

  Preferences prefs;
  if (!prefs.begin(kSetupPrefsNamespace, false)) {
    return;
  }

  const String storedPin = prefs.getString(kSetupPinKey, "");
  if (isSixDigitPin(storedPin)) {
    storedPin.toCharArray(setupPin, sizeof(setupPin));
    setupPinConfigured = true;
    setupUnlocked = false;
    prefs.end();
    return;
  }

  if (storedPin.length() > 0) {
    prefs.remove(kSetupPinKey);
  }

  prefs.end();
}

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
  margin: 0 0 0.9rem;
  font-size: 1.75rem;
  line-height: 1.15;
}

.fw-form {
  margin: 0.75rem 0 0;
}

.fw-field {
  margin: 0.6rem 0 0;
}

.fw-label {
  display: block;
  margin: 0 0 0.25rem;
  font-weight: 700;
}

.fw-input,
.fw-select,
.fw-button {
  height: 2rem;
  min-height: 2rem;
  border-style: solid;
  border-width: 2px;
  border-radius: 0;
  font: inherit;
}

.fw-input,
.fw-select {
  display: block;
  width: 100%;
  padding: 0.25rem 0.4rem;
  color: #000;
  background: #fff;
  border-color: #404040 #fff #fff #404040;
}

.fw-select {
  appearance: none;
  -webkit-appearance: none;
  padding-right: 1.8rem;
  background-image:
      linear-gradient(45deg, transparent 50%, #000 50%),
      linear-gradient(135deg, #000 50%, transparent 50%);
  background-position:
      calc(100% - 0.85rem) 0.8rem,
      calc(100% - 0.55rem) 0.8rem;
  background-size: 0.3rem 0.3rem, 0.3rem 0.3rem;
  background-repeat: no-repeat;
}

.fw-button {
  margin-top: 0.75rem;
  padding: 0.25rem 0.75rem;
  color: #000;
  background: #c0c0c0;
  border-color: #fff #404040 #404040 #fff;
  font-weight: 700;
}

.fw-button:active {
  border-color: #404040 #fff #fff #404040;
}

.fw-config-row .fw-button {
  margin-top: 0;
}

.fw-error {
  padding: 0.5rem;
  color: #fff;
  background: #800000;
  font-weight: 700;
}

.fw-config-notice {
  margin-bottom: 1rem;
}

.fw-link {
  color: #000080;
  font-weight: 700;
}

/* Generic FarmWhisper configuration control layout.
 * Each row has a full-width label/explanation line, then a control area and an
 * action button column. Text-only rows can span the full width.
 */
.fw-config-list {
  display: flex;
  flex-direction: column;
  gap: 0.9rem;
}

.fw-config-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 0.35rem 0.5rem;
  align-items: end;
}

.fw-config-label {
  grid-column: 1 / -1;
  font-weight: bold;
}

.fw-config-control {
  min-width: 0;
}

.fw-config-action {
  white-space: nowrap;
}

.fw-config-control .fw-input,
.fw-config-control .fw-select {
  width: 100%;
  box-sizing: border-box;
}

.fw-config-note {
  grid-column: 1 / -1;
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

void redirectToRoot() {
  sendNoStore();
  setupServer->sendHeader("Location", "/");
  setupServer->send(303, "text/plain", "");
}

const char *setupLockStateText() {
  if (!setupPinConfigured) {
    return "OPEN";
  }

  return setupUnlocked ? "UNLOCKED" : "LOCKED";
}

String setupUnlockPageHtml(
    const FWWiFiSetupWeb::SetupStatus &status,
    bool badPin) {
  String body;
  body.reserve(2200);
  body += R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>FarmWhisper Setup Unlock</title>
  <link rel="stylesheet" href="/setup.css">
</head>
<body class="fw-page">
  <main class="fw-window">
    <h1 class="fw-titlebar">FarmWhisper Setup Unlock</h1>
    <div class="fw-content">
      <p class="fw-intro">Enter the local setup PIN to continue.</p>
)HTML";

  if (badPin) {
    body += R"HTML(      <p class="fw-error">Incorrect PIN.</p>
)HTML";
  }

  body += R"HTML(      <section class="fw-section" aria-labelledby="fw-unlock-title">
        <h2 class="fw-section-title" id="fw-unlock-title">Local setup lock</h2>
        <p>A local setup PIN is configured on this device.</p>
        <form class="fw-form" method="post" action="/unlock">
          <div class="fw-field">
            <label class="fw-label" for="fw-pin">Setup PIN</label>
            <input class="fw-input" id="fw-pin" name="pin" type="password"
                   inputmode="numeric" pattern="[0-9]{6}" maxlength="6"
                   autocomplete="off" required autofocus>
          </div>
          <button class="fw-button" type="submit">Unlock setup</button>
        </form>
      </section>
      <dl class="fw-status-grid">
        <dt>Device ID</dt>
        <dd>)HTML";
  body += status.deviceId;
  body += R"HTML(</dd>
        <dt>Device alias</dt>
        <dd>)HTML";
  appendHtmlEscapedString(body, fwDeviceAlias());
  body += R"HTML(</dd>
        <dt>SSID</dt>
        <dd>)HTML";
  body += status.ssid;
  body += R"HTML(</dd>
        <dt>IP</dt>
        <dd>)HTML";
  body += status.ip.toString();
  body += R"HTML(</dd>
        <dt>Remaining</dt>
        <dd>)HTML";
  body += String(status.remainingS);
  body += R"HTML( s</dd>
      </dl>
      <p class="fw-actions">
        <a class="fw-link" href="/status">View setup status JSON</a>
      </p>
    </div>
  </main>
</body>
</html>
)HTML";

  return body;
}

void handleSetupCss() {
  sendNoStore();
  setupServer->send(200, "text/css", kSetupCss);
}

String setupRootPageHtml(
    const FWWiFiSetupWeb::SetupStatus &status,
    const char *noticeMessage = nullptr,
    bool noticeIsError = false) {
  /*
   * Setup PINs are optional.
   *
   * If no PIN is stored in NVS, setup opens directly. If a PIN is stored,
   * setup starts locked until the PIN is entered. Saving an empty PIN clears
   * the stored PIN and returns setup to the open/default state.
   */
  String body;
  body.reserve(4600);
  body += R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>FarmWhisper Setup</title>
  <link rel="stylesheet" href="/setup.css">
</head>
<body class="fw-page">
  <main class="fw-window">
    <h1 class="fw-titlebar">FarmWhisper Setup</h1>
    <div class="fw-content">
      <section class="fw-section" aria-labelledby="fw-config-title">
        <h2 class="fw-section-title" id="fw-config-title">FarmWhisper configuration</h2>
)HTML";

  if (noticeMessage != nullptr) {
    body += noticeIsError
        ? R"HTML(        <p class="fw-error fw-config-notice">)HTML"
        : R"HTML(        <p class="fw-note fw-config-notice">)HTML";
    appendHtmlEscapedString(body, noticeMessage);
    body += R"HTML(</p>
)HTML";
  }

  body += R"HTML(        <div class="fw-config-list">
          <form class="fw-config-row" method="post" action="/pin">
            <label class="fw-config-label" for="fw-new-pin">New setup PIN</label>
            <div class="fw-config-control">
              <input class="fw-input" id="fw-new-pin" name="pin" type="password"
                     inputmode="numeric" pattern="[0-9]{6}" maxlength="6"
                     autocomplete="new-password">
            </div>
            <button class="fw-button fw-config-action" type="submit">Update</button>
            <div class="fw-config-note">Use exactly 6 digits, or leave blank to clear the PIN.</div>
          </form>

          <form class="fw-config-row" method="post" action="/alias">
            <label class="fw-config-label" for="fw-alias">Device alias</label>
            <div class="fw-config-control">
              <input class="fw-input" id="fw-alias" name="alias" type="text"
                     maxlength="32" value=")HTML";
  appendHtmlEscapedString(body, fwDeviceAlias());
  body += R"HTML(" autocomplete="off">
            </div>
            <button class="fw-button fw-config-action" type="submit">Update</button>
            <div class="fw-config-note">Leave blank to restore the default alias.</div>
          </form>

          <form class="fw-config-row" method="post" action="/transport-mode">
            <label class="fw-config-label" for="fw-transport-mode">Transport mode</label>
            <div class="fw-config-control">
              <select class="fw-select" id="fw-transport-mode" name="mode">
)HTML";
  appendTransportModeOptions(body);
  body += R"HTML(
              </select>
            </div>
            <button class="fw-button fw-config-action" type="submit">Update</button>
            <div class="fw-config-note">Bluetooth LE choices enable discovery advertising, the Device Information Service, and the FarmWhisper Configuration Service. Telemetry, notifications, streaming, and OTA are not implemented.</div>
          </form>

          <form class="fw-config-row" method="post" action="/radio-profile">
            <label class="fw-config-label" for="fw-radio-profile">Radio profile</label>
            <div class="fw-config-control">
              <select class="fw-select" id="fw-radio-profile" name="profile">
)HTML";
  appendRadioProfileOptions(body);
  body += R"HTML(
              </select>
            </div>
            <button class="fw-button fw-config-action" type="submit">Update</button>
            <div class="fw-config-note">Radio settings are managed by firmware profiles.</div>
          </form>

          <form class="fw-config-row" method="post" action="/diagnostic-flood">
            <div class="fw-config-label">Diagnostic flood</div>
            <div class="fw-config-control">Send packets for 3 minutes</div>
            <button class="fw-button fw-config-action" type="submit">Start</button>
          </form>

          <div class="fw-config-row">
            <div class="fw-config-label">Sensor calibration</div>
            <div class="fw-config-note">Not implemented yet.</div>
          </div>
        </div>
        <p class="fw-actions">
          <a class="fw-link" href="/status">View setup status JSON</a>
        </p>
      </section>

      <section class="fw-section" aria-labelledby="fw-status-title">
        <h2 class="fw-section-title" id="fw-status-title">Device status</h2>
        <dl class="fw-status-grid">
          <dt>AP smoke/setup</dt>
          <dd>)HTML";
  body += status.apSmokeActive ? "ON" : "OFF";
  body += R"HTML(</dd>
          <dt>Setup HTTP</dt>
          <dd>)HTML";
  body += status.setupHttpActive ? "ON" : "OFF";
  body += R"HTML(</dd>
          <dt>Setup lock</dt>
          <dd>)HTML";
  body += setupLockStateText();
  body += R"HTML(</dd>
          <dt>Device ID</dt>
          <dd>)HTML";
  body += status.deviceId;
  body += R"HTML(</dd>
          <dt>Device alias</dt>
          <dd>)HTML";
  appendHtmlEscapedString(body, fwDeviceAlias());
  body += R"HTML(</dd>
          <dt>SSID</dt>
          <dd>)HTML";
  body += status.ssid;
  body += R"HTML(</dd>
          <dt>IP</dt>
          <dd>)HTML";
  body += status.ip.toString();
  body += R"HTML(</dd>
          <dt>Stations</dt>
          <dd>)HTML";
  body += String(static_cast<unsigned int>(status.stations));
  body += R"HTML(</dd>
          <dt>Age</dt>
          <dd>)HTML";
  body += String(status.ageS);
  body += R"HTML( s</dd>
          <dt>Timeout</dt>
          <dd>)HTML";
  body += String(status.timeoutS);
  body += R"HTML( s</dd>
          <dt>Remaining</dt>
          <dd>)HTML";
  body += String(status.remainingS);
  body += R"HTML( s</dd>
        </dl>
      </section>
    </div>
  </main>
</body>
</html>
)HTML";

  return body;
}

void handleSetupRoot() {
  const FWWiFiSetupWeb::SetupStatus status = currentStatus();
  String body;

  if (!setupPinConfigured || setupUnlocked) {
    const bool noticeIsError = setupNoticeIsError;
    const String notice = setupNotice;
    clearSetupNotice();
    body = setupRootPageHtml(
        status,
        notice.length() == 0 ? nullptr : notice.c_str(),
        noticeIsError);
  } else {
    body = setupUnlockPageHtml(status, false);
  }

  sendNoStore();
  setupServer->send(200, "text/html", body);
}

void handleSetupUnlock() {
  if (!setupPinConfigured) {
    redirectToRoot();
    return;
  }

  const String pin = setupServer->arg("pin");
  if (pin == setupPin) {
    setupUnlocked = true;
    redirectToRoot();
    return;
  }

  const String body = setupUnlockPageHtml(currentStatus(), true);
  sendNoStore();
  setupServer->send(403, "text/html", body);
}

void handleSetupAliasChange() {
  if (setupPinConfigured && !setupUnlocked) {
    sendNoStore();
    setupServer->send(
        403, "text/plain", "FarmWhisper setup is locked");
    return;
  }

  String alias = setupServer->arg("alias");
  alias.trim();

  if (alias.length() == 0) {
    fwClearDeviceAlias();
    setSetupNotice(
        "Device alias cleared. The default alias is now active.");
    redirectToRoot();
    return;
  }

  if (!fwSetDeviceAlias(alias.c_str())) {
    sendNoStore();
    setupServer->send(
        400,
        "text/plain",
        "Device alias must be blank or 1-32 printable characters.");
    return;
  }

  setSetupNotice(
      String("Device alias saved as \"") + fwDeviceAlias() +
      "\". This setting remains active until changed.");
  redirectToRoot();
}

void handleSetupTransportModeChange() {
  if (setupPinConfigured && !setupUnlocked) {
    sendNoStore();
    setupServer->send(
        403, "text/plain", "FarmWhisper setup is locked");
    return;
  }

  String modeKey = setupServer->arg("mode");
  modeKey.trim();

  if (!fwSetSelectedTransportModeByKey(modeKey.c_str())) {
    sendNoStore();
    setupServer->send(
        400,
        "text/plain",
        "Unknown FarmWhisper transport mode.");
    return;
  }

  const FwTransportModeId selectedId = fwSelectedTransportModeId();
  if (!fwTransportModeUsesBluetoothLe(selectedId)) {
    setSetupNotice(
        String("Transport mode set to ") +
        fwTransportModeName(selectedId) +
        ". Bluetooth LE advertising and services will stop. "
        "This setting remains active until changed.");
  } else if (fwTransportModeUsesLoRa(selectedId)) {
    setSetupNotice(
        String("Transport mode set to ") +
        fwTransportModeName(selectedId) +
        ". Bluetooth LE discovery, device information, and "
        "configuration services will run alongside LoRa. "
        "This setting remains active until changed.");
  } else {
    setSetupNotice(
        String("Transport mode set to ") +
        fwTransportModeName(selectedId) +
        ". Bluetooth LE discovery, device information, and "
        "configuration services will start. "
        "This setting remains active until changed.");
  }

  redirectToRoot();
}

void handleSetupRadioProfileChange() {
  if (setupPinConfigured && !setupUnlocked) {
    sendNoStore();
    setupServer->send(
        403, "text/plain", "FarmWhisper setup is locked");
    return;
  }

  String profileKey = setupServer->arg("profile");
  profileKey.trim();

  if (!fwSetSelectedRadioProfileByKey(profileKey.c_str())) {
    sendNoStore();
    setupServer->send(
        400,
        "text/plain",
        "Unknown FarmWhisper radio profile.");
    return;
  }

  const FwRadioProfileId selectedId = fwSelectedRadioProfileId();
  setSetupNotice(
      String("Radio profile set to ") +
      fwRadioProfileName(selectedId) +
      ". This setting remains active until changed.");
  redirectToRoot();
}

void handleSetupDiagnosticFlood() {
  if (setupPinConfigured && !setupUnlocked) {
    sendNoStore();
    setupServer->send(
        403, "text/plain", "FarmWhisper setup is locked");
    return;
  }

  const FwTransportModeId selectedId = fwSelectedTransportModeId();
  if (!fwTransportModeUsesLoRa(selectedId)) {
    setSetupNotice(
        "Diagnostic flood not started. "
        "The selected transport mode does not include LoRa.",
        true);
    redirectToRoot();
    return;
  }

  if (!FWRadio::startDiagnosticBurst(Serial)) {
    setSetupNotice(
        "Diagnostic flood is already running.",
        true);
    redirectToRoot();
    return;
  }

  setSetupNotice(
      "Diagnostic flood started. "
      "Sending packets for about 3 minutes.");
  redirectToRoot();
}

void handleSetupPinChange() {
  if (setupPinConfigured && !setupUnlocked) {
    sendNoStore();
    setupServer->send(
        403, "text/plain", "FarmWhisper setup is locked");
    return;
  }

  const String pin = setupServer->arg("pin");

  if (pin.length() == 0) {
    if (!clearSetupPinFromNvs()) {
      const String body = setupRootPageHtml(
          currentStatus(), "Could not clear PIN from device storage.", true);
      sendNoStore();
      setupServer->send(500, "text/html", body);
      return;
    }

    clearSetupPinFromRam();
    setSetupNotice(
        "PIN cleared. Setup will open directly until a new PIN is saved.");
    redirectToRoot();
    return;
  }

  if (!isSixDigitPin(pin)) {
    const String body = setupRootPageHtml(
        currentStatus(), "PIN must be blank or exactly 6 digits.", true);
    sendNoStore();
    setupServer->send(400, "text/html", body);
    return;
  }

  char newPin[sizeof(setupPin)];
  pin.toCharArray(newPin, sizeof(newPin));

  if (!saveSetupPinToNvs(newPin)) {
    const String body = setupRootPageHtml(
        currentStatus(), "Could not save PIN to device storage.", true);
    sendNoStore();
    setupServer->send(500, "text/html", body);
    return;
  }

  strncpy(setupPin, newPin, sizeof(setupPin));
  setupPin[sizeof(setupPin) - 1] = '\0';
  setupPinConfigured = true;
  setupUnlocked = true;
  setSetupNotice(
      "PIN saved. Future setup sessions will require it.");
  redirectToRoot();
}

void handleSetupStatus() {
  const FWWiFiSetupWeb::SetupStatus status = currentStatus();

  String body;
  body.reserve(850);
  body += "{\n";
  body += "  \"apSmoke\": ";
  body += status.apSmokeActive ? "true" : "false";
  body += ",\n  \"setupHttp\": ";
  body += status.setupHttpActive ? "true" : "false";
  body += ",\n  \"setupUnlocked\": ";
  body += setupUnlocked ? "true" : "false";
  body += ",\n  \"setupLocked\": ";
  body += (setupPinConfigured && !setupUnlocked) ? "true" : "false";
  body += ",\n  \"setupPinConfigured\": ";
  body += setupPinConfigured ? "true" : "false";
  body += ",\n  \"setupPinStorage\": \"nvs_optional\"";
  body +=
      ",\n  \"setupPinRecovery\": "
      "\"button_hold_until_red_5_flashes\"";
  body += ",\n  \"deviceId\": \"";
  body += status.deviceId;
  body += "\"";
  body += ",\n  \"deviceAlias\": \"";
  appendJsonEscapedString(body, fwDeviceAlias());
  body += "\"";

  const FwRadioProfileId selectedRadioProfileId =
      fwSelectedRadioProfileId();
  body += ",\n  \"radioProfileKey\": \"";
  appendJsonEscapedString(
      body, fwRadioProfileKey(selectedRadioProfileId));
  body += "\"";
  body += ",\n  \"radioProfileName\": \"";
  appendJsonEscapedString(
      body, fwRadioProfileName(selectedRadioProfileId));
  body += "\"";

  const FwTransportModeId selectedTransportModeId =
      fwSelectedTransportModeId();
  body += ",\n  \"transportModeKey\": \"";
  appendJsonEscapedString(
      body, fwTransportModeKey(selectedTransportModeId));
  body += "\"";
  body += ",\n  \"transportModeName\": \"";
  appendJsonEscapedString(
      body, fwTransportModeName(selectedTransportModeId));
  body += "\"";

  body += ",\n  \"ssid\": \"";
  body += status.ssid;
  body += "\"";
  body += ",\n  \"ip\": \"";
  body += status.ip.toString();
  body += "\"";
  body += ",\n  \"stations\": ";
  body += String(static_cast<unsigned int>(status.stations));
  body += ",\n  \"ageS\": ";
  body += String(status.ageS);
  body += ",\n  \"timeoutS\": ";
  body += String(status.timeoutS);
  body += ",\n  \"remainingS\": ";
  body += String(status.remainingS);
  body += "\n}\n";

  sendNoStore();
  setupServer->send(200, "application/json", body);
}

void handleSetupNotFound() {
  setupServer->send(
      404, "text/plain", "FarmWhisper setup placeholder: not found");
}

}  // namespace

namespace FWWiFiSetupWeb {

void registerRoutes(WebServer &server, StatusProvider statusProvider) {
  setupServer = &server;
  provideStatus = statusProvider;
  loadSetupPinFromNvs();

  server.on("/", HTTP_GET, handleSetupRoot);
  server.on("/setup.css", HTTP_GET, handleSetupCss);
  server.on("/status", HTTP_GET, handleSetupStatus);
  server.on("/unlock", HTTP_POST, handleSetupUnlock);
  server.on("/alias", HTTP_POST, handleSetupAliasChange);
  server.on(
      "/transport-mode", HTTP_POST, handleSetupTransportModeChange);
  server.on(
      "/radio-profile", HTTP_POST, handleSetupRadioProfileChange);
  server.on(
      "/diagnostic-flood", HTTP_POST, handleSetupDiagnosticFlood);
  server.on("/pin", HTTP_POST, handleSetupPinChange);
  server.onNotFound(handleSetupNotFound);
}

void resetSession() {
  setupUnlocked = false;
  clearSetupNotice();
}

bool clearStoredPin() {
  if (!clearSetupPinFromNvs()) {
    return false;
  }

  clearSetupPinFromRam();
  setSetupNotice(
      "PIN cleared by physical recovery. Setup is open.");
  return true;
}

}  // namespace FWWiFiSetupWeb
