#include "fw_wifi_status.h"

#include <WiFi.h>

namespace {

constexpr const char *kApSmokeSsid = "FarmWhisper-Setup";
constexpr uint8_t kApSmokeChannel = 6;
constexpr uint8_t kApSmokeMaxClients = 2;

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

bool apSmokeActive = false;

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

void forceWifiOff() {
  /*
   * Keep this conservative while WiFi bring-up is still diagnostic-only.
   * Clearing scan data and disabling the radio prevents diagnostics from
   * changing the validated component-test baseline after they return.
   */
  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  apSmokeActive = false;
}

void printApStatus(Stream &out) {
  if (!apSmokeActive) {
    return;
  }

  out.print("[wifi] ap ssid=\"");
  out.print(kApSmokeSsid);
  out.print("\" ip=");
  out.print(WiFi.softAPIP());
  out.print(" ch=");
  out.print(kApSmokeChannel);
  out.print(" stations=");
  out.println(WiFi.softAPgetStationNum());
}

} // namespace

namespace FWWiFiStatus {

void begin() {
  /*
   * Disable persistence before touching mode so diagnostics do not write
   * network state or credentials to flash.
   */
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
  out.println(apSmokeActive ? "ON" : "OFF");

  printApStatus(out);
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

void toggleApSmoke(Stream &out) {
  out.println();

  if (apSmokeActive) {
    out.println("[wifi] AP smoke stop");
    forceWifiOff();
    printStatus(out);
    return;
  }

  /*
   * Manual AP smoke test only. This proves the ESP32 can advertise a setup
   * network before any portal/server/credential logic is introduced.
   */
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
      kApSmokeSsid,
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

  out.println("[wifi] AP smoke start");
  out.println("[wifi] no web server, no DNS, no captive portal, no credentials");
  printStatus(out);
}

} // namespace FWWiFiStatus
