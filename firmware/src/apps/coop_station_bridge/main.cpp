#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <Wire.h>

#include "fw_ble_telemetry.h"
#include "fw_device_config.h"
#include "fw_packet.h"
#include "fw_radio.h"

namespace {

constexpr uint32_t kSerialBaud = 115200;
constexpr int8_t kDisplaySdaPin = 17;
constexpr int8_t kDisplaySclPin = 18;
constexpr int8_t kDisplayResetPin = 21;
constexpr int8_t kDisplayVextPin = 36;
constexpr uint8_t kDisplayAddress = 0x3C;
constexpr int16_t kDisplayWidth = 128;
constexpr int16_t kDisplayHeight = 64;
constexpr uint32_t kScanSeconds = 5;
constexpr uint32_t kRetryDelayMs = 2000;
constexpr char kCoopProfileKey[] = "us-long-range";
constexpr char kCoopChannelKey[] = "ch09";

Adafruit_SSD1306 display(kDisplayWidth, kDisplayHeight, &Wire, kDisplayResetPin);
BLEScan *scanner = nullptr;
bool displayReady = false;
bool radioReady = false;
uint32_t scanPass = 0;
uint32_t decodedCount = 0;
uint32_t malformedCount = 0;
uint32_t relayPassCount = 0;
uint32_t relayFailCount = 0;

bool haveReading = false;
FWPacket::Telemetry lastTelemetry = {};
int lastBleRssi = 0;
char lastSource[sizeof("FWP-000000")] = "FWP-000000";
const char *lastRelayState = "WAIT";

bool haveRelayed = false;
uint8_t lastRelayedSource[3] = {};
uint32_t lastRelayedSequence = 0;
uint32_t lastAttemptMs = 0;

bool sameReading(
    const FWPacket::Telemetry &telemetry,
    const uint8_t sourceId[3],
    uint32_t sequence) {
  return telemetry.sequence == sequence &&
      telemetry.sourceId[0] == sourceId[0] &&
      telemetry.sourceId[1] == sourceId[1] &&
      telemetry.sourceId[2] == sourceId[2];
}

bool alreadyRelayed(const FWPacket::Telemetry &telemetry) {
  return haveRelayed &&
      sameReading(telemetry, lastRelayedSource, lastRelayedSequence);
}

void rememberRelayed(const FWPacket::Telemetry &telemetry) {
  lastRelayedSource[0] = telemetry.sourceId[0];
  lastRelayedSource[1] = telemetry.sourceId[1];
  lastRelayedSource[2] = telemetry.sourceId[2];
  lastRelayedSequence = telemetry.sequence;
  haveRelayed = true;
}

void drawStatus() {
  if (!displayReady) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("FW Coop Bridge ");
  display.println(lastRelayState);

  if (!haveReading) {
    display.println();
    display.println("BLE -> LoRa");
    display.print(kCoopProfileKey);
    display.print("/");
    display.println(kCoopChannelKey);
    display.println("Waiting for FWP...");
    display.display();
    return;
  }

  display.println(lastSource);
  display.print("Seq ");
  display.print(lastTelemetry.sequence);
  display.print(" BLE ");
  display.println(lastBleRssi);
  display.print("Distance ");
  display.print(lastTelemetry.distanceMm);
  display.println(" mm");
  display.print("Fill ");
  display.print(lastTelemetry.fillPermille / 10);
  display.print('.');
  display.print(lastTelemetry.fillPermille % 10);
  display.println('%');
  display.print("Batt ");
  display.print(lastTelemetry.batteryMillivolts);
  display.println(" mV");
  display.print("TX ");
  display.print(relayPassCount);
  display.print(" fail ");
  display.println(relayFailCount);
  display.display();
}

void printBleReading(const FWPacket::Telemetry &telemetry, int rssi) {
  char source[sizeof("FWP-000000")] = {};
  FWPacket::formatSourceId(telemetry.sourceId, source, sizeof(source));
  Serial.print("[coop] BLE source=");
  Serial.print(source);
  Serial.print(" sequence=");
  Serial.print(telemetry.sequence);
  Serial.print(" distanceMm=");
  Serial.print(telemetry.distanceMm);
  Serial.print(" fillPermille=");
  Serial.print(telemetry.fillPermille);
  Serial.print(" batteryMv=");
  Serial.print(telemetry.batteryMillivolts);
  Serial.print(" flags=0x");
  Serial.print(telemetry.flags, HEX);
  Serial.print(" rssi=");
  Serial.println(rssi);
}

bool relayTelemetry(const FWPacket::Telemetry &telemetry) {
  lastAttemptMs = millis();
  Serial.print("[coop] relay begin sequence=");
  Serial.println(telemetry.sequence);

  const bool passed = FWRadio::transmitTelemetry(telemetry, Serial);
  if (passed) {
    ++relayPassCount;
    rememberRelayed(telemetry);
    lastRelayState = "TX PASS";
    Serial.print("[coop] relay PASS sequence=");
    Serial.println(telemetry.sequence);
  } else {
    ++relayFailCount;
    lastRelayState = "TX FAIL";
    Serial.print("[coop] relay FAIL sequence=");
    Serial.print(telemetry.sequence);
    Serial.print(" code=");
    Serial.println(FWRadio::lastTxResult());
  }
  drawStatus();
  return passed;
}

class BridgeCallbacks final : public BLEAdvertisedDeviceCallbacks {
 public:
  void onResult(BLEAdvertisedDevice device) override {
    if (!device.haveManufacturerData()) {
      return;
    }

    const auto manufacturerData = device.getManufacturerData();
    FWPacket::Telemetry telemetry = {};
    const uint8_t *bytes =
        reinterpret_cast<const uint8_t *>(manufacturerData.c_str());

    if (!FWBleTelemetry::decodeManufacturerData(
            bytes, manufacturerData.length(), telemetry)) {
      if (manufacturerData.length() == FWBleTelemetry::kManufacturerDataSize) {
        ++malformedCount;
      }
      return;
    }

    ++decodedCount;
    const int rssi = device.getRSSI();
    lastTelemetry = telemetry;
    lastBleRssi = rssi;
    haveReading = true;
    FWPacket::formatSourceId(telemetry.sourceId, lastSource, sizeof(lastSource));

    if (alreadyRelayed(telemetry)) {
      return;
    }

    if (lastAttemptMs != 0 && millis() - lastAttemptMs < kRetryDelayMs) {
      return;
    }

    printBleReading(telemetry, rssi);
    relayTelemetry(telemetry);
  }
};

BridgeCallbacks callbacks;

void beginDisplay() {
#if defined(FW_COOP_STATION_XIAO_S3_WIO_SX1262)
  displayReady = false;
  Serial.println("[display] no onboard OLED on XIAO S3 coop station");
  return;
#else
  pinMode(kDisplayVextPin, OUTPUT);
  digitalWrite(kDisplayVextPin, LOW);
  delay(100);
  Wire.begin(kDisplaySdaPin, kDisplaySclPin);
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, kDisplayAddress);
  if (!displayReady) {
    Serial.println("[display] SSD1306 init failed; bridge remains active");
    return;
  }
  display.setTextWrap(false);
  drawStatus();
#endif
}

void beginRadio() {
  fwLoadDeviceConfig();
  const bool profileSaved = fwSetSelectedRadioProfileByKey(kCoopProfileKey);
  const bool channelSaved = fwSetSelectedRadioChannelByKey(kCoopChannelKey);
  Serial.print("[coop] radio selection profile=");
  Serial.print(kCoopProfileKey);
  Serial.print(" saved=");
  Serial.print(profileSaved ? "yes" : "no");
  Serial.print(" channel=");
  Serial.print(kCoopChannelKey);
  Serial.print(" saved=");
  Serial.println(channelSaved ? "yes" : "no");
  radioReady = FWRadio::beginDiagnostic(Serial);
  lastRelayState = radioReady ? "READY" : "RADIO ERR";
}

void beginBleScanner() {
  BLEDevice::init("FW-Coop-Bridge");
  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(&callbacks, false);
  scanner->setActiveScan(true);
  scanner->setInterval(160);
  scanner->setWindow(120);
  Serial.println("[coop] BLE scanner ready; shared adapter active");
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(1200);
  Serial.println();
#if defined(FW_COOP_STATION_XIAO_S3_WIO_SX1262)
  Serial.println("FarmWhisper XIAO S3 + Wio-SX1262 coop station v005-port");
#else
  Serial.println("FarmWhisper Heltec V4 coop station v005");
#endif
  Serial.println("Shared BLE telemetry -> shared FWPacket -> shared FWRadio");
  beginDisplay();
  beginRadio();
  beginBleScanner();
  drawStatus();
}

void loop() {
  ++scanPass;
  decodedCount = 0;
  malformedCount = 0;
  BLEScanResults results = scanner->start(kScanSeconds, false);
  Serial.print("[coop] pass=");
  Serial.print(scanPass);
  Serial.print(" unique=");
  Serial.print(results.getCount());
  Serial.print(" decoded=");
  Serial.print(decodedCount);
  Serial.print(" malformed=");
  Serial.print(malformedCount);
  Serial.print(" txPass=");
  Serial.print(relayPassCount);
  Serial.print(" txFail=");
  Serial.println(relayFailCount);
  scanner->clearResults();
  drawStatus();
  delay(500);
}
