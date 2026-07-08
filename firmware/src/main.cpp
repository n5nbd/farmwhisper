#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <VL53L1X.h>

// FarmWhisper Heltec WiFi LoRa 32 V4 R2/R8 base connector contract.
// Header J3 bottom-up:
// 1 GND
// 2 3V3
// 3 3V3 / aux 3V3
// 4 GPIO37 spare/questionable GPIO
// 5 GPIO46 product I2C SCL
// 6 GPIO45 product I2C SDA
// 7 GPIO42 big button, active LOW
// 8 GPIO41 NeoPixel data

static constexpr uint8_t PIN_PRODUCT_I2C_SDA = 45;
static constexpr uint8_t PIN_PRODUCT_I2C_SCL = 46;
static constexpr uint8_t PIN_BUTTON = 42;
static constexpr uint8_t FW_PIN_NEOPIXEL = 41;

static constexpr uint32_t SERIAL_BAUD = 115200;

static constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;
static constexpr uint32_t BUTTON_LONG_PRESS_MS = 1200;
static constexpr uint32_t BUTTON_MULTI_PRESS_GAP_MS = 450;

static constexpr uint32_t BUTTON_SHORT_FLASH_MS = 150;
static constexpr uint32_t BUTTON_LONG_FLASH_MS = 450;
static constexpr uint32_t BUTTON_MULTI_FLASH_MS = 350;

static constexpr uint32_t HEARTBEAT_MS = 1000;
static constexpr uint32_t TOF_POLL_MS = 100;

static constexpr uint8_t STABILITY_WINDOW_SIZE = 5;
static constexpr uint16_t STABILITY_MAX_SPAN_MM = 25;

Adafruit_NeoPixel pixel(1, FW_PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
VL53L1X tof;

enum class ComponentStatus {
  Booting,
  TofInitFailed,
  TofTimeout,
  TofShady,
  TofWarming,
  TofUnstable,
  TofStable
};

enum class ButtonOverlay {
  None,
  ShortPress,
  LongPress,
  DoublePress,
  TriplePress
};

static ComponentStatus componentStatus = ComponentStatus::Booting;

volatile uint32_t rawButtonIrqCount = 0;

static bool lastRawButton = HIGH;
static bool debouncedButton = HIGH;
static uint32_t rawButtonChangedAtMs = 0;

static uint32_t pressCount = 0;
static uint32_t longPressCount = 0;
static uint32_t doublePressCount = 0;
static uint32_t triplePressCount = 0;

static uint32_t buttonPressedAtMs = 0;
static bool buttonLongPressReported = false;

static uint8_t pendingShortPresses = 0;
static uint32_t lastShortPressReleaseMs = 0;

static uint32_t buttonFlashUntilMs = 0;
static ButtonOverlay buttonOverlay = ButtonOverlay::None;

static bool tofReady = false;
static uint32_t tofValidCount = 0;
static uint32_t tofShadyCount = 0;
static uint32_t tofTimeoutCount = 0;
static uint16_t lastValidTofMm = 0;
static bool hasLastValidTof = false;

static uint16_t stabilityWindow[STABILITY_WINDOW_SIZE] = {};
static uint8_t stabilityCount = 0;
static uint8_t stabilityWriteIndex = 0;

static uint32_t lastHeartbeatMs = 0;
static uint32_t lastTofPollMs = 0;

void IRAM_ATTR onButtonFalling() {
  rawButtonIrqCount++;
}

const char *buttonText(bool value) {
  return value == LOW ? "LOW/pressed" : "HIGH/released";
}

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

void setPixel(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void updateNeoPixel() {
  const uint32_t now = millis();

  if (now < buttonFlashUntilMs) {
    switch (buttonOverlay) {
      case ButtonOverlay::ShortPress:
        setPixel(40, 40, 40);  // white
        return;

      case ButtonOverlay::LongPress:
        setPixel(0, 40, 40);   // cyan
        return;

      case ButtonOverlay::DoublePress:
        setPixel(0, 0, 45);    // blue
        return;

      case ButtonOverlay::TriplePress:
        setPixel(45, 0, 45);   // magenta
        return;

      case ButtonOverlay::None:
        break;
    }
  } else {
    buttonOverlay = ButtonOverlay::None;
  }

  const bool blinkFast = ((now / 125) % 2) == 0;
  const bool blinkSlow = ((now / 500) % 2) == 0;

  switch (componentStatus) {
    case ComponentStatus::Booting:
      setPixel(0, 0, 30);
      break;

    case ComponentStatus::TofInitFailed:
      setPixel(blinkFast ? 45 : 0, 0, 0);
      break;

    case ComponentStatus::TofTimeout:
      setPixel(blinkSlow ? 45 : 0, 0, 0);
      break;

    case ComponentStatus::TofShady:
      setPixel(blinkFast ? 35 : 0, 0, blinkFast ? 35 : 0);
      break;

    case ComponentStatus::TofWarming:
      setPixel(0, 0, blinkSlow ? 35 : 8);
      break;

    case ComponentStatus::TofUnstable:
      setPixel(blinkSlow ? 35 : 6, blinkSlow ? 20 : 3, 0);
      break;

    case ComponentStatus::TofStable:
      setPixel(0, 35, 0);
      break;
  }
}

bool scanProductI2cFor(uint8_t expectedAddr) {
  Serial.println();
  Serial.println("[i2c] Scanning product I2C bus GPIO45 SDA / GPIO46 SCL");

  bool foundExpected = false;
  uint8_t foundCount = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    const uint8_t error = Wire.endTransmission();

    if (error == 0) {
      foundCount++;
      Serial.print("[i2c] Found device at 0x");
      if (addr < 16) {
        Serial.print('0');
      }
      Serial.println(addr, HEX);

      if (addr == expectedAddr) {
        foundExpected = true;
      }
    }
  }

  Serial.print("[i2c] Scan complete, devices=");
  Serial.print(foundCount);
  Serial.print(", expected 0x");
  if (expectedAddr < 16) {
    Serial.print('0');
  }
  Serial.print(expectedAddr, HEX);
  Serial.print(" present=");
  Serial.println(foundExpected ? "yes" : "no");

  return foundExpected;
}

void addValidStabilitySample(uint16_t mm) {
  stabilityWindow[stabilityWriteIndex] = mm;
  stabilityWriteIndex = (stabilityWriteIndex + 1) % STABILITY_WINDOW_SIZE;

  if (stabilityCount < STABILITY_WINDOW_SIZE) {
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

  return stabilityCount == STABILITY_WINDOW_SIZE && spanMm <= STABILITY_MAX_SPAN_MM;
}

void printStabilitySummary() {
  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = computeStability(avgMm, spanMm);

  Serial.print(" stable=");
  if (stabilityCount < STABILITY_WINDOW_SIZE) {
    Serial.print("warming");
  } else {
    Serial.print(stable ? "yes" : "no");
  }

  Serial.print(" stableAvgMm=");
  Serial.print(avgMm);
  Serial.print(" stableSpanMm=");
  Serial.print(spanMm);
}

void triggerButtonOverlay(ButtonOverlay overlay, uint32_t durationMs) {
  buttonOverlay = overlay;
  buttonFlashUntilMs = millis() + durationMs;
}

void printButtonEventCounters() {
  Serial.print(" pressCount=");
  Serial.print(pressCount);
  Serial.print(" longPressCount=");
  Serial.print(longPressCount);
  Serial.print(" doublePressCount=");
  Serial.print(doublePressCount);
  Serial.print(" triplePressCount=");
  Serial.print(triplePressCount);
  Serial.print(" pendingShortPresses=");
  Serial.print(pendingShortPresses);
  Serial.print(" rawIrqCount=");
  Serial.print(rawButtonIrqCount);
}

void fireDoublePressEvent() {
  doublePressCount++;
  triggerButtonOverlay(ButtonOverlay::DoublePress, BUTTON_MULTI_FLASH_MS);

  Serial.print("[button] doublePress");
  printButtonEventCounters();
  Serial.println();
}

void fireTriplePressEvent() {
  triplePressCount++;
  triggerButtonOverlay(ButtonOverlay::TriplePress, BUTTON_MULTI_FLASH_MS);

  Serial.print("[button] triplePress");
  printButtonEventCounters();
  Serial.println();
}

void finishPendingShortPressSequenceIfReady(uint32_t now) {
  if (debouncedButton == LOW) {
    return;
  }

  if (pendingShortPresses == 0) {
    return;
  }

  if ((now - lastShortPressReleaseMs) < BUTTON_MULTI_PRESS_GAP_MS) {
    return;
  }

  if (pendingShortPresses == 2) {
    fireDoublePressEvent();
  } else if (pendingShortPresses == 1) {
    Serial.print("[button] singleShortPressSequence");
    printButtonEventCounters();
    Serial.println();
  } else if (pendingShortPresses >= 3) {
    fireTriplePressEvent();
  }

  pendingShortPresses = 0;
}

void updateButton() {
  const uint32_t now = millis();
  const bool rawButton = digitalRead(PIN_BUTTON);

  if (rawButton != lastRawButton) {
    lastRawButton = rawButton;
    rawButtonChangedAtMs = now;

    Serial.print("[button] raw=");
    Serial.print(buttonText(rawButton));
    Serial.print(" rawIrqCount=");
    Serial.println(rawButtonIrqCount);
  }

  if ((now - rawButtonChangedAtMs) >= BUTTON_DEBOUNCE_MS && rawButton != debouncedButton) {
    const bool previousDebouncedButton = debouncedButton;
    debouncedButton = rawButton;

    Serial.print("[button] debounced=");
    Serial.print(buttonText(debouncedButton));

    if (debouncedButton == LOW) {
      pressCount++;
      buttonPressedAtMs = now;
      buttonLongPressReported = false;
      triggerButtonOverlay(ButtonOverlay::ShortPress, BUTTON_SHORT_FLASH_MS);

      printButtonEventCounters();
    } else if (previousDebouncedButton == LOW) {
      const uint32_t heldMs = now - buttonPressedAtMs;

      Serial.print(" heldMs=");
      Serial.print(heldMs);
      Serial.print(" longPressSeen=");
      Serial.print(buttonLongPressReported ? "yes" : "no");

      if (!buttonLongPressReported) {
        pendingShortPresses++;
        lastShortPressReleaseMs = now;

        if (pendingShortPresses >= 3) {
          fireTriplePressEvent();
          pendingShortPresses = 0;
        } else {
          printButtonEventCounters();
        }
      } else {
        pendingShortPresses = 0;
        printButtonEventCounters();
      }
    }

    Serial.println();
  }

  if (debouncedButton == LOW && !buttonLongPressReported) {
    const uint32_t heldMs = now - buttonPressedAtMs;

    if (heldMs >= BUTTON_LONG_PRESS_MS) {
      buttonLongPressReported = true;
      longPressCount++;
      pendingShortPresses = 0;
      triggerButtonOverlay(ButtonOverlay::LongPress, BUTTON_LONG_FLASH_MS);

      Serial.print("[button] longPress heldMs=");
      Serial.print(heldMs);
      printButtonEventCounters();
      Serial.println();
    }
  }

  finishPendingShortPressSequenceIfReady(now);
}

void pollTof() {
  if (!tofReady) {
    componentStatus = ComponentStatus::TofInitFailed;
    return;
  }

  const uint32_t now = millis();
  if ((now - lastTofPollMs) < TOF_POLL_MS) {
    return;
  }
  lastTofPollMs = now;

  const uint16_t distanceMm = tof.read();

  if (tof.timeoutOccurred()) {
    tofTimeoutCount++;
    componentStatus = ComponentStatus::TofTimeout;

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
    return;
  }

  const VL53L1X::RangeStatus rangeStatus = tof.ranging_data.range_status;
  const char *rangeStatusName = VL53L1X::rangeStatusToString(rangeStatus);

  if (rangeStatus != VL53L1X::RangeValid) {
    tofShadyCount++;
    componentStatus = ComponentStatus::TofShady;

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
    return;
  }

  tofValidCount++;
  lastValidTofMm = distanceMm;
  hasLastValidTof = true;
  addValidStabilitySample(distanceMm);

  uint16_t avgMm = 0;
  uint16_t spanMm = 0;
  const bool stable = computeStability(avgMm, spanMm);

  if (stabilityCount < STABILITY_WINDOW_SIZE) {
    componentStatus = ComponentStatus::TofWarming;
  } else if (stable) {
    componentStatus = ComponentStatus::TofStable;
  } else {
    componentStatus = ComponentStatus::TofUnstable;
  }

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
  Serial.print(buttonText(digitalRead(PIN_BUTTON)));
  Serial.print(" rawIrqCount=");
  Serial.print(rawButtonIrqCount);
  Serial.print(" pressCount=");
  Serial.print(pressCount);
  Serial.print(" longPressCount=");
  Serial.print(longPressCount);
  Serial.print(" doublePressCount=");
  Serial.print(doublePressCount);
  Serial.print(" triplePressCount=");
  Serial.print(triplePressCount);
  Serial.print(" pendingShortPresses=");
  Serial.print(pendingShortPresses);
  Serial.print(" tofReady=");
  Serial.print(tofReady ? "yes" : "no");
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
  if (stabilityCount < STABILITY_WINDOW_SIZE) {
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
  if ((now - lastHeartbeatMs) < HEARTBEAT_MS) {
    return;
  }
  lastHeartbeatMs = now;

  printStatusSnapshot("[heartbeat]");
}

void printSerialHelp() {
  Serial.println();
  Serial.println("[serial] Commands:");
  Serial.println("[serial]   h or ?  help");
  Serial.println("[serial]   s       print status snapshot");
  Serial.println("[serial]   i       rescan product I2C bus");
  Serial.println("[serial]   r       reset runtime diagnostics");
}

void resetRuntimeDiagnostics() {
  noInterrupts();
  rawButtonIrqCount = 0;
  interrupts();

  pressCount = 0;
  longPressCount = 0;
  doublePressCount = 0;
  triplePressCount = 0;

  pendingShortPresses = 0;
  lastShortPressReleaseMs = 0;
  buttonLongPressReported = false;

  tofValidCount = 0;
  tofShadyCount = 0;
  tofTimeoutCount = 0;
  lastValidTofMm = 0;
  hasLastValidTof = false;

  for (uint8_t i = 0; i < STABILITY_WINDOW_SIZE; i++) {
    stabilityWindow[i] = 0;
  }
  stabilityCount = 0;
  stabilityWriteIndex = 0;

  buttonOverlay = ButtonOverlay::None;
  buttonFlashUntilMs = 0;

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
      scanProductI2cFor(0x29);
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

  Serial.begin(SERIAL_BAUD);
  delay(300);

  Serial.println();
  Serial.println("===== FarmWhisper component validation baseline =====");
  Serial.println("[boot] Heltec WiFi LoRa 32 V4 R2/R8");
  Serial.println("[boot] USB CDC serial enabled");
  Serial.println("[boot] Product I2C: SDA GPIO45, SCL GPIO46");
  Serial.println("[boot] Button: GPIO42 active LOW, raw IRQ + debounced app events");
  Serial.println("[boot] Button events: short press, long press, double press, triple press");
  Serial.println("[boot] NeoPixel: GPIO41 status model");
  Serial.println("[boot] Serial diagnostics: h/? help, s status, i i2c scan, r reset counters");
  Serial.println("[boot] Display/OLED disabled");
  Serial.println("[boot] LoRa/WiFi/NVS/app calibration not enabled");

  pixel.begin();
  pixel.setBrightness(40);
  setPixel(0, 0, 30);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  lastRawButton = digitalRead(PIN_BUTTON);
  debouncedButton = lastRawButton;
  rawButtonChangedAtMs = millis();

  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), onButtonFalling, FALLING);

  Wire.begin(PIN_PRODUCT_I2C_SDA, PIN_PRODUCT_I2C_SCL);
  Wire.setClock(400000);

  const bool foundTof = scanProductI2cFor(0x29);

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
  updateButton();
  pollTof();
  updateNeoPixel();
  printHeartbeat();
}