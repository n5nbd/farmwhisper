#include "fw_wifi_status.h"
#include "fw_wifi_setup_web.h"

#include <WebServer.h>
#include <WiFi.h>

#include <esp_mac.h>

namespace {

constexpr const char *kApSmokeSsidPrefix = "FarmWhisper-";
constexpr const char *kDeviceIdPrefix = "FWP-";
constexpr uint8_t kMacSuffixHexChars = 6;

char macSuffix[kMacSuffixHexChars + 1] = "000000";
char apSmokeSsid[sizeof("FarmWhisper-") + kMacSuffixHexChars] = "FarmWhisper-000000";
char deviceId[sizeof("FWP-") + kMacSuffixHexChars] = "FWP-000000";

constexpr uint8_t kApSmokeChannel = 6;
constexpr uint8_t kApSmokeMaxClients = 2;

// Manual AP smoke-test timeout.
constexpr uint32_t kApSmokeTimeoutMs = 5UL * 60UL * 1000UL;

/*
 * FarmWhisper setup AP address.
 *
 * Keep this explicit instead of relying on the ESP32 Arduino default
 * 192.168.4.1 address. Future captive-portal and setup UI work should build
 * on this address contract.
 */
const IPAddress kApSmokeIp(10, 10, 10, 10);
const IPAddress kApSmokeGateway(10, 10, 10, 10);
const IPAddress kApSmokeNetmask(255, 255, 255, 0);


WebServer setupServer(80);

bool apSmokeActive = false;
bool setupHttpActive = false;
bool setupHttpRoutesConfigured = false;
uint32_t apSmokeStartedMs = 0;

const char *wifiModeText(wifi_mode_t mode) {
  switch (mode) {
  case WIFI_OFF:
    return "OFF";
  case WIFI_STA:
    return "STA";
  case WIFI_AP:
    return "AP";
  case WIFI_AP_STA:
    return "AP_STA";
  default:
    return "UNKNOWN";
  }
}

const char *wifiStatusText(wl_status_t status) {
  switch (status) {
  case WL_IDLE_STATUS:
    return "IDLE";
  case WL_NO_SSID_AVAIL:
    return "NO_SSID";
  case WL_SCAN_COMPLETED:
    return "SCAN_COMPLETED";
  case WL_CONNECTED:
    return "CONNECTED";
  case WL_CONNECT_FAILED:
    return "CONNECT_FAILED";
  case WL_CONNECTION_LOST:
    return "CONNECTION_LOST";
  case WL_DISCONNECTED:
    return "DISCONNECTED";
  default:
    /*
     * ESP32 Arduino commonly reports 255 when WiFi is OFF or unavailable.
     * Treat that as expected during component validation rather than as a
     * mysterious unknown error.
     */
    if (static_cast<int>(status) == 255) {
      return "OFF_OR_UNAVAILABLE";
    }
    return "UNKNOWN";
  }
}

const char *wifiAuthText(wifi_auth_mode_t authMode) {
  return authMode == WIFI_AUTH_OPEN ? "open" : "secured";
}

void buildDeviceIdentity() {
  uint8_t mac[6] = {0};

  /*
   * Use the ESP32 base WiFi STA MAC as the device identity seed. The setup AP
   * SSID and FarmWhisper product ID use the last three bytes as six uppercase
   * hex characters. That keeps both strings short enough for printed labels,
   * laser marking, serial logs, and field setup.
   */
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
    snprintf(
        macSuffix,
        sizeof(macSuffix),
        "%02X%02X%02X",
        mac[3],
        mac[4],
        mac[5]);
  } else {
    snprintf(macSuffix, sizeof(macSuffix), "000000");
  }

  snprintf(
      apSmokeSsid,
      sizeof(apSmokeSsid),
      "%s%s",
      kApSmokeSsidPrefix,
      macSuffix);

  snprintf(
      deviceId,
      sizeof(deviceId),
      "%s%s",
      kDeviceIdPrefix,
      macSuffix);
}

uint32_t apSmokeAgeMs() {
  if (!apSmokeActive) {
    return 0;
  }

  return millis() - apSmokeStartedMs;
}

uint32_t apSmokeRemainingMs() {
  if (!apSmokeActive) {
    return 0;
  }

  const uint32_t ageMs = apSmokeAgeMs();
  if (ageMs >= kApSmokeTimeoutMs) {
    return 0;
  }

  return kApSmokeTimeoutMs - ageMs;
}


FWWiFiSetupWeb::SetupStatus currentSetupWebStatus() {
  return {
      apSmokeActive,
      setupHttpActive,
      deviceId,
      apSmokeSsid,
      WiFi.softAPIP(),
      static_cast<uint8_t>(WiFi.softAPgetStationNum()),
      apSmokeAgeMs() / 1000UL,
      kApSmokeTimeoutMs / 1000UL,
      apSmokeRemainingMs() / 1000UL,
  };
}

void startSetupHttpServer(Stream &out) {
  if (setupHttpActive) {
    return;
  }

  /*
   * This server is deliberately tiny and manual-only. It proves that the AP
   * can host a page at the setup address without adding DNS, redirect logic,
   * network-credential forms, general config writes, or boot-time WiFi behavior.
   */
  if (!setupHttpRoutesConfigured) {
    FWWiFiSetupWeb::registerRoutes(setupServer, currentSetupWebStatus);
    setupHttpRoutesConfigured = true;
  }

  setupServer.begin();
  setupHttpActive = true;

  out.print("[wifi] setup http start url=http://");
  out.print(kApSmokeIp);
  out.println("/");
}

void stopSetupHttpServer() {
  if (!setupHttpActive) {
    return;
  }

  /*
   * Stop accepting HTTP traffic before the AP radio is torn down. The WebServer
   * object remains allocated for the next manual AP smoke/setup session.
   */
  setupServer.close();
  FWWiFiSetupWeb::resetSession();
  setupHttpActive = false;
}

void forceWifiOff() {
  /*
   * Keep this conservative while WiFi bring-up is still diagnostic-only.
   * Clearing scan data and disabling the radio prevents diagnostics from
   * changing the validated component-test baseline after they return.
   */
  stopSetupHttpServer();
  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  apSmokeActive = false;
  apSmokeStartedMs = 0;
}

void printApStatus(Stream &out) {
  if (!apSmokeActive) {
    return;
  }

  out.print("[wifi] ap deviceId=");
  out.print(deviceId);
  out.print(" ssid=\"");
  out.print(apSmokeSsid);
  out.print("\" ip=");
  out.print(WiFi.softAPIP());
  out.print(" ch=");
  out.print(kApSmokeChannel);
  out.print(" stations=");
  out.print(WiFi.softAPgetStationNum());
  out.print(" http=");
  out.print(setupHttpActive ? "ON" : "OFF");
  out.print(" ageS=");
  out.print((millis() - apSmokeStartedMs) / 1000UL);
  out.print(" timeoutS=");
  out.println(kApSmokeTimeoutMs / 1000UL);
}

} // namespace

namespace FWWiFiStatus {

void begin() {
  /*
   * Disable persistence before touching mode so diagnostics do not write
   * network state or credentials to flash.
   */
  buildDeviceIdentity();
  WiFi.persistent(false);
  forceWifiOff();
}

void printStatus(Stream &out) {
  const wifi_mode_t mode = WiFi.getMode();
  const wl_status_t status = WiFi.status();

  out.print("[wifi] mode=");
  out.print(wifiModeText(mode));

  if (mode == WIFI_AP) {
    out.print(" staStatus=N/A_AP_MODE");
  } else {
    out.print(" status=");
    out.print(wifiStatusText(status));
    out.print("(");
    out.print(static_cast<int>(status));
    out.print(")");
  }

  out.print(" apSmoke=");
  out.print(apSmokeActive ? "ON" : "OFF");
  out.print(" setupHttp=");
  out.print(setupHttpActive ? "ON" : "OFF");
  out.print(" deviceId=");
  out.println(deviceId);

  printApStatus(out);
}

void service(Stream &out) {
  if (!apSmokeActive) {
    return;
  }

  if (setupHttpActive) {
    setupServer.handleClient();
  }

  if ((millis() - apSmokeStartedMs) < kApSmokeTimeoutMs) {
    return;
  }

  out.println();
  out.println("[wifi] AP smoke timeout; stopping AP");
  forceWifiOff();
  printStatus(out);
}

void scanOnce(Stream &out) {
  out.println();
  out.println("[wifi] scan begin");
  printStatus(out);

  /*
   * STA mode is enabled only for this synchronous manual scan. The command
   * must always end by forcing WiFi back OFF. If the AP smoke test was active,
   * this command intentionally tears it down before scanning.
   */
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  stopSetupHttpServer();
  apSmokeActive = false;
  delay(100);

  const int networkCount = WiFi.scanNetworks(false, true);

  if (networkCount < 0) {
    out.print("[wifi] scan failed code=");
    out.println(networkCount);
    forceWifiOff();
    out.println("[wifi] scan done; mode=OFF");
    printStatus(out);
    return;
  }

  out.print("[wifi] networks=");
  out.println(networkCount);

  for (int i = 0; i < networkCount; ++i) {
    out.print("[wifi] ");
    if ((i + 1) < 10) {
      out.print('0');
    }
    out.print(i + 1);
    out.print(" ssid=\"");
    out.print(WiFi.SSID(i));
    out.print("\" rssi=");
    out.print(WiFi.RSSI(i));
    out.print(" ch=");
    out.print(WiFi.channel(i));
    out.print(" auth=");
    out.println(wifiAuthText(WiFi.encryptionType(i)));
  }

  forceWifiOff();
  out.println("[wifi] scan done; mode=OFF");
  printStatus(out);
}

void startApSetup(Stream &out) {
  out.println();

  if (apSmokeActive) {
    apSmokeStartedMs = millis();
    out.println("[wifi] AP setup already active; timeout refreshed");
    printStatus(out);
    return;
  }

  /*
   * Manual setup AP only. This proves the ESP32 can advertise and serve a
   * setup placeholder before any DNS, portal, network-credential, or general config storage logic is
   * introduced. It is safe to call from the physical button path.
   */
  buildDeviceIdentity();
  WiFi.persistent(false);
  WiFi.scanDelete();
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP);

  if (!WiFi.softAPConfig(kApSmokeIp, kApSmokeGateway, kApSmokeNetmask)) {
    out.println("[wifi] AP smoke config failed");
    forceWifiOff();
    printStatus(out);
    return;
  }

  const bool started = WiFi.softAP(
      apSmokeSsid,
      nullptr,
      kApSmokeChannel,
      false,
      kApSmokeMaxClients);

  if (!started) {
    out.println("[wifi] AP smoke start failed");
    forceWifiOff();
    printStatus(out);
    return;
  }

  apSmokeActive = true;
  apSmokeStartedMs = millis();

  out.println("[wifi] AP smoke start");
  startSetupHttpServer(out);
  out.println("[wifi] no DNS, no captive portal, no credentials, no storage");
  printStatus(out);
}

void refreshApSetupTimeout(Stream &out) {
  out.println();

  if (!apSmokeActive) {
    out.println("[wifi] AP setup timeout refresh ignored; AP is OFF");
    printStatus(out);
    return;
  }

  apSmokeStartedMs = millis();
  out.println("[wifi] AP setup timeout refreshed");
  printStatus(out);
}

void toggleApSmoke(Stream &out) {
  if (apSmokeActive) {
    out.println();
    out.println("[wifi] AP smoke stop");
    forceWifiOff();
    printStatus(out);
    return;
  }

  startApSetup(out);
}


bool clearSetupPinWithRecovery(Stream &out) {
  out.println();

  if (!apSmokeActive) {
    out.println("[wifi] setup PIN clear ignored; setup AP is OFF");
    printStatus(out);
    return false;
  }

  if (!FWWiFiSetupWeb::clearStoredPin()) {
    out.println("[wifi] setup PIN clear failed");
    printStatus(out);
    return false;
  }

  apSmokeStartedMs = millis();

  out.println("[wifi] setup PIN cleared; setup now opens without a PIN");
  out.println("[wifi] setup session opened; AP timeout refreshed");
  printStatus(out);
  return true;
}

} // namespace FWWiFiStatus
