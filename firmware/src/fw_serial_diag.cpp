#include "fw_serial_diag.h"

#include <Arduino.h>

#include "fw_button.h"
#include "fw_config.h"
#include "fw_expansion_gpio.h"
#include "fw_pins.h"
#include "fw_product_i2c.h"
#include "fw_radio.h"
#include "fw_status_pixel.h"
#include "fw_tof.h"
#include "fw_tof_stability.h"
#include "fw_types.h"
#include "fw_wifi_status.h"

namespace {

uint32_t lastHeartbeatMs = 0;

const char *statusText(ComponentStatus status) {
  switch (status) {
    case ComponentStatus::Booting:
      return "BOOTING";
    case ComponentStatus::TofInitFailed:
      return "TOF_INIT_FAILED";
    case ComponentStatus::TofTimeout:
      return "TOF_TIMEOUT";
    case ComponentStatus::TofShady:
      return "TOF_SHADY";
    case ComponentStatus::TofWarming:
      return "TOF_WARMING";
    case ComponentStatus::TofUnstable:
      return "TOF_UNSTABLE";
    case ComponentStatus::TofStable:
      return "TOF_STABLE";
    default:
      return "UNKNOWN";
  }
}

void processSerialCommand(char command) {
  switch (command) {
    case 'h':
    case 'H':
    case '?':
      FWSerialDiag::printHelp();
      break;

    case 's':
    case 'S':
      FWSerialDiag::printStatusSnapshot("[serial]");
      break;

    case 'i':
    case 'I':
      FWProductI2C::scanFor(0x29);
      break;

    case 'g':
    case 'G':
      FWExpansionGPIO::printSmokeStatus("[gpio]");
      break;

    case 'v':
    case 'V':
      FWToF::toggleVerboseLogging();
      Serial.print("[serial] ToF verbose logging=");
      Serial.println(FWToF::verboseLogging() ? "on" : "off");
      break;

    case 'a':
    case 'A':
      FWWiFiStatus::toggleApSmoke(Serial);
      break;

    case 'x':
    case 'X':
      FWWiFiStatus::printStatus(Serial);
      break;

    case 'w':
    case 'W':
      FWWiFiStatus::scanOnce(Serial);
      break;


    case 't':
    case 'T':
      FWRadio::transmitDiagnostic(Serial);
      break;

    case 'l':
    case 'L':
      FWRadio::beginDiagnostic(Serial);
      break;

    case 'r':
    case 'R':
      FWSerialDiag::resetRuntimeDiagnostics();
      break;

    default:
      Serial.print("[serial] Unknown command: ");
      Serial.println(command);
      Serial.println("[serial] Type h or ? for help");
      break;
  }
}

}  // namespace

namespace FWSerialDiag {

void printBootBanner() {
  Serial.println();
  Serial.println("===== FarmWhisper component validation baseline =====");
#if defined(FW_BOARD_XIAO_C6)
  Serial.println("[boot] Seeed XIAO ESP32-C6 FW100 hardware port");
  Serial.println("[boot] USB CDC serial enabled");
  Serial.println("[boot] Product I2C: SDA GPIO22, SCL GPIO23");
  Serial.println("[boot] Switched 3V3 rail: GPIO21 active LOW, held ON for parity port");
  Serial.println("[boot] Button: GPIO1 active LOW, raw IRQ + debounced app events");
#else
  Serial.println("[boot] Heltec WiFi LoRa 32 V4 R2/R8");
  Serial.println("[boot] USB CDC serial enabled");
  Serial.println("[boot] Product I2C: SDA GPIO45, SCL GPIO46");
  Serial.println("[boot] Button: GPIO42 active LOW, raw IRQ + debounced app events");
#endif
  Serial.println("[boot] Button events: short press, long press, double press, triple press starts/refreshes setup AP");
#if defined(FW_BOARD_XIAO_C6)
  Serial.println("[boot] NeoPixel: GPIO2 status model");
  Serial.println("[boot] Battery measurement deferred; expansion GPIO smoke test unavailable on this target");
#else
  Serial.println("[boot] NeoPixel: GPIO41 status model");
  Serial.println("[boot] GPIO37: dedicated battery measurement enable; GPIO38/39/40: expansion GPIO smoke test as INPUT_PULLUP");
#endif
  Serial.println("[boot] Serial diagnostics: h/? help, s status, i i2c scan, g gpio smoke, v tof verbose, a wifi AP, x wifi status, w wifi scan, l LoRa init, t LoRa TX, r reset counters");
  Serial.println("[boot] Display/OLED disabled");
#if defined(FW_BOARD_XIAO_C6)
  Serial.println("[boot] LoRa hardware unavailable; l/t diagnostics are no-op");
#else
  Serial.println("[boot] LoRa inactive until explicit serial l diagnostic");
#endif
  Serial.println("[boot] WiFi customer join/app calibration not enabled");
}

void printStatusSnapshot(const char *prefix) {
  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = FWToFStability::compute(avgMm, spanMm);

  Serial.print(prefix);
  Serial.print(" ms=");
  Serial.print(millis());
  Serial.print(" status=");
  Serial.print(statusText(FWToF::status()));
  Serial.print(" rawButton=");
  Serial.print(FWButton::buttonText(digitalRead(FWPin::BigButton)));
  Serial.print(" rawIrqCount=");
  Serial.print(FWButton::rawIrqCount());
#if defined(FW_BOARD_XIAO_C6)
  Serial.print(" rail21=");
  Serial.print(digitalRead(FWPin::SensorRailEnable) == LOW ? "LOW/on" : "HIGH/off");
#else
  Serial.print(" batteryEnable37=");
  Serial.print(digitalRead(FWPin::BatteryMeasureEnable) == HIGH ? "HIGH/measuring" : "LOW/inactive");
  Serial.print(" gpio38=");
  Serial.print(digitalRead(FWPin::ExpansionGpio38) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio39=");
  Serial.print(digitalRead(FWPin::ExpansionGpio39) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio40=");
  Serial.print(digitalRead(FWPin::ExpansionGpio40) == LOW ? "LOW/grounded" : "HIGH/open");
#endif
  Serial.print(" pressCount=");
  Serial.print(FWButton::pressCount());
  Serial.print(" longPressCount=");
  Serial.print(FWButton::longPressCount());
  Serial.print(" doublePressCount=");
  Serial.print(FWButton::doublePressCount());
  Serial.print(" triplePressCount=");
  Serial.print(FWButton::triplePressCount());
  Serial.print(" pendingShortPresses=");
  Serial.print(FWButton::pendingShortPresses());
  Serial.print(" radio=");
  Serial.print(FWRadio::stateName());
  Serial.print(" radioResult=");
  Serial.print(FWRadio::lastResult());
  Serial.print(" radioSelectedProfile=");
  Serial.print(FWRadio::selectedProfileKey());
  Serial.print(" radioAppliedProfile=");
  Serial.print(FWRadio::appliedProfileKey());
  Serial.print(" radioTxCount=");
  Serial.print(FWRadio::txCount());
  Serial.print(" radioLastTxResult=");
  Serial.print(FWRadio::lastTxResult());
  Serial.print(" tofReady=");
  Serial.print(FWToF::ready() ? "yes" : "no");
  Serial.print(" tofVerbose=");
  Serial.print(FWToF::verboseLogging() ? "on" : "off");
  Serial.print(" tofValid=");
  Serial.print(FWToF::validCount());
  Serial.print(" tofShady=");
  Serial.print(FWToF::shadyCount());
  Serial.print(" tofTimeout=");
  Serial.print(FWToF::timeoutCount());
  Serial.print(" lastValidMm=");
  if (FWToF::hasLastValid()) {
    Serial.print(FWToF::lastValidMm());
  } else {
    Serial.print("none");
  }
  Serial.print(" stable=");
  if (FWToFStability::isWarming()) {
    Serial.print("warming");
  } else {
    Serial.print(stable ? "yes" : "no");
  }
  Serial.print(" stableAvgMm=");
  Serial.print(avgMm);
  Serial.print(" stableSpanMm=");
  Serial.println(spanMm);
}

void printHeartbeat() {
  const uint32_t now = millis();
  if ((now - lastHeartbeatMs) < FWConfig::HeartbeatMs) {
    return;
  }
  lastHeartbeatMs = now;

  printStatusSnapshot("[heartbeat]");
}

void printHelp() {
  Serial.println();
  Serial.println("[serial] Commands:");
  Serial.println("[serial]   h or ?  help");
  Serial.println("[serial]   s       print status snapshot");
  Serial.println("[serial]   i       rescan product I2C bus");
#if defined(FW_BOARD_XIAO_C6)
  Serial.println("[serial]   g       expansion GPIO smoke test unavailable on this target");
#else
  Serial.println("[serial]   g       print GPIO37/38/39/40 smoke-test states");
#endif
  Serial.println("[serial]   v       toggle verbose per-sample ToF logging");
  Serial.println("[serial]   a       toggle WiFi AP smoke test");
  Serial.println("[serial]   button triple press starts/refreshes WiFi setup AP");
  Serial.println("[serial]   x       print WiFi radio status without scanning");
  Serial.println("[serial]   w       scan WiFi networks, then return WiFi OFF");
#if defined(FW_BOARD_XIAO_C6)
  Serial.println("[serial]   l       LoRa unavailable on XIAO ESP32-C6 (no-op)");
  Serial.println("[serial]   t       LoRa unavailable on XIAO ESP32-C6 (no-op)");
#else
  Serial.println("[serial]   l       initialize onboard SX1262 using selected profile");
  Serial.println("[serial]   t       transmit one LoRa diagnostic packet");
#endif
  Serial.println("[serial]   r       reset runtime diagnostics");
}

void resetRuntimeDiagnostics() {
  FWButton::resetDiagnostics();

  FWToF::resetDiagnostics();

  FWStatusPixel::clearButtonOverlay();

  Serial.println("[serial] Runtime diagnostics reset");
  printStatusSnapshot("[serial]");
}

void handleCommands() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());

    if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
      continue;
    }

    processSerialCommand(c);

    // Keep commands single-character for now.
    while (Serial.available() > 0) {
      const char discard = static_cast<char>(Serial.peek());
      if (discard == '\r' || discard == '\n' || discard == ' ' || discard == '\t') {
        Serial.read();
      } else {
        break;
      }
    }
  }
}

}  // namespace FWSerialDiag
