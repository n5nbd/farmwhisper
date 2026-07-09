#include "fw_wifi_status.h"

#include <WiFi.h>

namespace {

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
  WiFi.scanDelete();
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
}

} // namespace

namespace FWWiFiStatus {

void begin() {
  WiFi.persistent(false);
  forceWifiOff();
}

void printStatus(Stream &out) {
  const wl_status_t status = WiFi.status();

  out.print("[wifi] mode=");
  out.print(wifiModeText(WiFi.getMode()));
  out.print(" status=");
  out.print(wifiStatusText(status));
  out.print("(");
  out.print(static_cast<int>(status));
  out.println(")");
}

void scanOnce(Stream &out) {
  out.println();
  out.println("[wifi] scan begin");
  printStatus(out);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
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

} // namespace FWWiFiStatus
