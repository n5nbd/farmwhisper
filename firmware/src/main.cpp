#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>

#include "fw_button.h"
#include "fw_config.h"
#include "fw_pins.h"
#include "fw_product_i2c.h"
#include "fw_status_pixel.h"
#include "fw_types.h"

VL53L1X tof;

static ComponentStatus componentStatus = ComponentStatus::Booting;

static bool tofReady = false;
static bool tofVerboseLogging = false;
static uint32_t tofValidCount = 0;
static uint32_t tofShadyCount = 0;
static uint32_t tofTimeoutCount = 0;
static uint16_t lastValidTofMm = 0;
static bool hasLastValidTof = false;

static uint16_t stabilityWindow[FWConfig::StabilityWindowSize] = {};
static uint8_t stabilityCount = 0;
static uint8_t stabilityWriteIndex = 0;

static uint32_t lastHeartbeatMs = 0;
static uint32_t lastTofPollMs = 0;

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

void addValidStabilitySample(uint16_t mm) {
  stabilityWindow[stabilityWriteIndex] = mm;
  stabilityWriteIndex = (stabilityWriteIndex + 1) % FWConfig::StabilityWindowSize;

  if (stabilityCount < FWConfig::StabilityWindowSize) {
    stabilityCount++;
  }
}

bool computeStability(uint16_t &avgMm, uint16_t &spanMm) {
  if (stabilityCount == 0) {
    avgMm = 0;
    spanMm = 0;
    return false;
  }

  uint32_t sum = 0;
  uint16_t minMm = stabilityWindow[0];
  uint16_t maxMm = stabilityWindow[0];

  for (uint8_t i = 0; i < stabilityCount; i++) {
    const uint16_t value = stabilityWindow[i];
    sum += value;

    if (value < minMm) {
      minMm = value;
    }

    if (value > maxMm) {
      maxMm = value;
    }
  }

  avgMm = static_cast<uint16_t>(sum / stabilityCount);
  spanMm = maxMm - minMm;

  return stabilityCount == FWConfig::StabilityWindowSize && spanMm <= FWConfig::StabilityMaxSpanMm;
}

void printStabilitySummary() {
  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = computeStability(avgMm, spanMm);

  Serial.print(" stable=");
  if (stabilityCount < FWConfig::StabilityWindowSize) {
    Serial.print("warming");
  } else {
    Serial.print(stable ? "yes" : "no");
  }

  Serial.print(" stableAvgMm=");
  Serial.print(avgMm);
  Serial.print(" stableSpanMm=");
  Serial.print(spanMm);
}

void pollTof() {
  if (!tofReady) {
    componentStatus = ComponentStatus::TofInitFailed;
    return;
  }

  const uint32_t now = millis();
  if ((now - lastTofPollMs) < FWConfig::TofPollMs) {
    return;
  }
  lastTofPollMs = now;

  const uint16_t distanceMm = tof.read();

  if (tof.timeoutOccurred()) {
    tofTimeoutCount++;
    componentStatus = ComponentStatus::TofTimeout;

    if (tofVerboseLogging) {
      Serial.print("[tof] timeout timeoutCount=");
      Serial.print(tofTimeoutCount);
      Serial.print(" lastValidMm=");
      if (hasLastValidTof) {
        Serial.print(lastValidTofMm);
      } else {
        Serial.print("none");
      }
      printStabilitySummary();
      Serial.println();
    }

    return;
  }

  const VL53L1X::RangeStatus rangeStatus = tof.ranging_data.range_status;
  const char *rangeStatusName = VL53L1X::rangeStatusToString(rangeStatus);

  if (rangeStatus != VL53L1X::RangeValid) {
    tofShadyCount++;
    componentStatus = ComponentStatus::TofShady;

    if (tofVerboseLogging) {
      Serial.print("[tof] ~");
      Serial.print(distanceMm);
      Serial.print(" mm status=");
      Serial.print(rangeStatusName);
      Serial.print(" shadyCount=");
      Serial.print(tofShadyCount);
      Serial.print(" lastValidMm=");
      if (hasLastValidTof) {
        Serial.print(lastValidTofMm);
      } else {
        Serial.print("none");
      }
      printStabilitySummary();
      Serial.println();
    }

    return;
  }

  tofValidCount++;
  lastValidTofMm = distanceMm;
  hasLastValidTof = true;
  addValidStabilitySample(distanceMm);

  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = computeStability(avgMm, spanMm);

  if (stabilityCount < FWConfig::StabilityWindowSize) {
    componentStatus = ComponentStatus::TofWarming;
  } else if (stable) {
    componentStatus = ComponentStatus::TofStable;
  } else {
    componentStatus = ComponentStatus::TofUnstable;
  }

  if (tofVerboseLogging) {
    Serial.print("[tof] ");
    Serial.print(distanceMm);
    Serial.print(" mm status=");
    Serial.print(rangeStatusName);
    Serial.print(" validCount=");
    Serial.print(tofValidCount);
    Serial.print(" lastValidMm=");
    Serial.print(lastValidTofMm);
    printStabilitySummary();
    Serial.println();
  }
}

void printStatusSnapshot(const char *prefix) {
  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = computeStability(avgMm, spanMm);

  Serial.print(prefix);
  Serial.print(" ms=");
  Serial.print(millis());
  Serial.print(" status=");
  Serial.print(statusText(componentStatus));
  Serial.print(" rawButton=");
  Serial.print(FWButton::buttonText(digitalRead(FWPin::BigButton)));
  Serial.print(" rawIrqCount=");
  Serial.print(FWButton::rawIrqCount());
  Serial.print(" gpio37=");
  Serial.print(digitalRead(FWPin::SpareGpio37) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio38=");
  Serial.print(digitalRead(FWPin::ExpansionGpio38) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio39=");
  Serial.print(digitalRead(FWPin::ExpansionGpio39) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio40=");
  Serial.print(digitalRead(FWPin::ExpansionGpio40) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" FWButton::pressCount()=");
  Serial.print(FWButton::pressCount());
  Serial.print(" FWButton::longPressCount()=");
  Serial.print(FWButton::longPressCount());
  Serial.print(" FWButton::doublePressCount()=");
  Serial.print(FWButton::doublePressCount());
  Serial.print(" FWButton::triplePressCount()=");
  Serial.print(FWButton::triplePressCount());
  Serial.print(" FWButton::pendingShortPresses()=");
  Serial.print(FWButton::pendingShortPresses());
  Serial.print(" tofReady=");
  Serial.print(tofReady ? "yes" : "no");
  Serial.print(" tofVerbose=");
  Serial.print(tofVerboseLogging ? "on" : "off");
  Serial.print(" tofValid=");
  Serial.print(tofValidCount);
  Serial.print(" tofShady=");
  Serial.print(tofShadyCount);
  Serial.print(" tofTimeout=");
  Serial.print(tofTimeoutCount);
  Serial.print(" lastValidMm=");
  if (hasLastValidTof) {
    Serial.print(lastValidTofMm);
  } else {
    Serial.print("none");
  }
  Serial.print(" stable=");
  if (stabilityCount < FWConfig::StabilityWindowSize) {
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

void printGpioSmokeStatus(const char *prefix) {
  Serial.print(prefix);
  Serial.print(" gpio37=");
  Serial.print(digitalRead(FWPin::SpareGpio37) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio38=");
  Serial.print(digitalRead(FWPin::ExpansionGpio38) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio39=");
  Serial.print(digitalRead(FWPin::ExpansionGpio39) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.print(" gpio40=");
  Serial.print(digitalRead(FWPin::ExpansionGpio40) == LOW ? "LOW/grounded" : "HIGH/open");
  Serial.println(" mode=INPUT_PULLUP expected=HIGH/open LOW/jumpered-to-GND");
}

void printSerialHelp() {
  Serial.println();
  Serial.println("[serial] Commands:");
  Serial.println("[serial]   h or ?  help");
  Serial.println("[serial]   s       print status snapshot");
  Serial.println("[serial]   i       rescan product I2C bus");
  Serial.println("[serial]   g       print GPIO37/38/39/40 smoke-test states");
  Serial.println("[serial]   v       toggle verbose per-sample ToF logging");
  Serial.println("[serial]   r       reset runtime diagnostics");
}

void resetRuntimeDiagnostics() {
  FWButton::resetDiagnostics();

  tofValidCount = 0;
  tofShadyCount = 0;
  tofTimeoutCount = 0;
  lastValidTofMm = 0;
  hasLastValidTof = false;

  for (uint8_t i = 0; i < FWConfig::StabilityWindowSize; i++) {
    stabilityWindow[i] = 0;
  }
  stabilityCount = 0;
  stabilityWriteIndex = 0;

  FWStatusPixel::clearButtonOverlay();

  if (tofReady) {
    componentStatus = ComponentStatus::TofWarming;
  } else {
    componentStatus = ComponentStatus::TofInitFailed;
  }

  Serial.println("[serial] Runtime diagnostics reset");
  printStatusSnapshot("[serial]");
}

void processSerialCommand(char command) {
  switch (command) {
    case 'h':
    case 'H':
    case '?':
      printSerialHelp();
      break;

    case 's':
    case 'S':
      printStatusSnapshot("[serial]");
      break;

    case 'i':
    case 'I':
      FWProductI2C::scanFor(0x29);
      break;

    case 'g':
    case 'G':
      printGpioSmokeStatus("[gpio]");
      break;

    case 'v':
    case 'V':
      tofVerboseLogging = !tofVerboseLogging;
      Serial.print("[serial] ToF verbose logging=");
      Serial.println(tofVerboseLogging ? "on" : "off");
      break;

    case 'r':
    case 'R':
      resetRuntimeDiagnostics();
      break;

    default:
      Serial.print("[serial] Unknown command: ");
      Serial.println(command);
      Serial.println("[serial] Type h or ? for help");
      break;
  }
}

void handleSerialCommands() {
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

void setup() {
  delay(1200);

  Serial.begin(FWConfig::SerialBaud);
  delay(300);

  Serial.println();
  Serial.println("===== FarmWhisper component validation baseline =====");
  Serial.println("[boot] Heltec WiFi LoRa 32 V4 R2/R8");
  Serial.println("[boot] USB CDC serial enabled");
  Serial.println("[boot] Product I2C: SDA GPIO45, SCL GPIO46");
  Serial.println("[boot] Button: GPIO42 active LOW, raw IRQ + debounced app events");
  Serial.println("[boot] Button events: short press, long press, double press, triple press");
  Serial.println("[boot] NeoPixel: GPIO41 status model");
  Serial.println("[boot] GPIO37/38/39/40: spare/expansion GPIO smoke test as INPUT_PULLUP");
  Serial.println("[boot] Serial diagnostics: h/? help, s status, i i2c scan, g gpio smoke, v tof verbose, r reset counters");
  Serial.println("[boot] Display/OLED disabled");
  Serial.println("[boot] LoRa/WiFi/NVS/app calibration not enabled");

  FWStatusPixel::begin();

  FWButton::begin();
  pinMode(FWPin::SpareGpio37, INPUT_PULLUP);
  pinMode(FWPin::ExpansionGpio38, INPUT_PULLUP);
  pinMode(FWPin::ExpansionGpio39, INPUT_PULLUP);
  pinMode(FWPin::ExpansionGpio40, INPUT_PULLUP);

  Wire.begin(FWPin::ProductI2cSda, FWPin::ProductI2cScl);
  Wire.setClock(400000);

  const bool foundTof = FWProductI2C::scanFor(0x29);

  Serial.println();
  Serial.println("[tof] Initializing VL53L1X");

  tof.setTimeout(500);

  if (!foundTof || !tof.init()) {
    tofReady = false;
    componentStatus = ComponentStatus::TofInitFailed;
    Serial.println("[tof] ERROR: VL53L1X init failed");
    printSerialHelp();
    return;
  }

  tof.setDistanceMode(VL53L1X::Long);
  tof.setMeasurementTimingBudget(50000);
  tof.startContinuous(100);
  lastTofPollMs = millis();

  tofReady = true;
  componentStatus = ComponentStatus::TofWarming;

  Serial.println("[tof] VL53L1X ready");
  Serial.println("[tof] Mode=Long timingBudgetUs=50000 continuousPeriodMs=100");
  Serial.println("[boot] Component validation loop started");

  printSerialHelp();
}

void loop() {
  handleSerialCommands();
  FWButton::update();
  pollTof();
  FWStatusPixel::update(componentStatus);
  printHeartbeat();
}