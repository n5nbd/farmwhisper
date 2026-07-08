#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <VL53L1X.h>

static constexpr int PIN_I2C_SDA = 45;
static constexpr int PIN_I2C_SCL = 46;

static constexpr int PIN_BUTTON = 42;  // active LOW
static constexpr int PIN_PIXEL  = 41;

static constexpr uint8_t PIXEL_COUNT = 1;
static constexpr uint8_t VL53L1X_ADDR = 0x29;

static constexpr unsigned long BUTTON_DEBOUNCE_MS = 75;

static constexpr uint8_t TOF_STABILITY_WINDOW = 5;
static constexpr uint16_t TOF_STABILITY_MAX_SPAN_MM = 25;

Adafruit_NeoPixel pixel(PIXEL_COUNT, PIN_PIXEL, NEO_GRB + NEO_KHZ800);
VL53L1X tof;

unsigned long lastPixelMs = 0;
unsigned long lastHeartbeatMs = 0;
unsigned long lastTofReportMs = 0;

uint8_t colorStep = 0;
bool lastRawButtonState = HIGH;
bool tofReady = false;

// Application-facing button state
uint32_t buttonPressCount = 0;
unsigned long lastAcceptedButtonPressMs = 0;

// ISR-owned button state
volatile uint32_t buttonRawIrqCount = 0;
volatile bool buttonIrqFlag = false;

// Application-facing ToF state
bool hasLastValidTof = false;
uint16_t lastValidTofMm = 0;
uint32_t tofValidCount = 0;
uint32_t tofShadyCount = 0;
uint32_t tofTimeoutCount = 0;

// ToF stability window, using valid samples only
uint16_t tofStableWindow[TOF_STABILITY_WINDOW] = {};
uint8_t tofStableWindowCount = 0;
uint8_t tofStableWindowNext = 0;
bool tofStable = false;
uint16_t tofStableMm = 0;
uint16_t tofStableSpanMm = 0;

void IRAM_ATTR onButtonInterrupt() {
  buttonRawIrqCount++;
  buttonIrqFlag = true;
}

void setPixelStep(uint8_t step) {
  switch (step % 6) {
    case 0:
      pixel.setPixelColor(0, pixel.Color(24, 0, 0));    // red
      break;
    case 1:
      pixel.setPixelColor(0, pixel.Color(0, 24, 0));    // green
      break;
    case 2:
      pixel.setPixelColor(0, pixel.Color(0, 0, 24));    // blue
      break;
    case 3:
      pixel.setPixelColor(0, pixel.Color(24, 12, 0));   // amber
      break;
    case 4:
      pixel.setPixelColor(0, pixel.Color(0, 24, 24));   // cyan
      break;
    default:
      pixel.setPixelColor(0, pixel.Color(24, 0, 24));   // magenta
      break;
  }

  pixel.show();
}

bool i2cProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

const char* formatLastValidTof() {
  static char buffer[20];

  if (hasLastValidTof) {
    snprintf(buffer, sizeof(buffer), "%u mm", lastValidTofMm);
  } else {
    snprintf(buffer, sizeof(buffer), "none");
  }

  return buffer;
}

const char* tofStableStateText() {
  if (tofStableWindowCount < TOF_STABILITY_WINDOW) {
    return "warming";
  }

  return tofStable ? "yes" : "no";
}

const char* formatStableTof() {
  static char buffer[24];

  if (tofStableWindowCount < TOF_STABILITY_WINDOW) {
    snprintf(buffer, sizeof(buffer), "warming");
  } else if (tofStable) {
    snprintf(buffer, sizeof(buffer), "%u mm", tofStableMm);
  } else {
    snprintf(buffer, sizeof(buffer), "~%u mm", tofStableMm);
  }

  return buffer;
}

void updateTofStability(uint16_t distanceMm) {
  tofStableWindow[tofStableWindowNext] = distanceMm;
  tofStableWindowNext = (tofStableWindowNext + 1) % TOF_STABILITY_WINDOW;

  if (tofStableWindowCount < TOF_STABILITY_WINDOW) {
    tofStableWindowCount++;
  }

  if (tofStableWindowCount < TOF_STABILITY_WINDOW) {
    tofStable = false;
    tofStableMm = distanceMm;
    tofStableSpanMm = 0;
    return;
  }

  uint16_t minMm = UINT16_MAX;
  uint16_t maxMm = 0;
  uint32_t sumMm = 0;

  for (uint8_t i = 0; i < TOF_STABILITY_WINDOW; i++) {
    uint16_t value = tofStableWindow[i];

    if (value < minMm) {
      minMm = value;
    }

    if (value > maxMm) {
      maxMm = value;
    }

    sumMm += value;
  }

  tofStableSpanMm = maxMm - minMm;
  tofStableMm = static_cast<uint16_t>((sumMm + (TOF_STABILITY_WINDOW / 2)) /
                                      TOF_STABILITY_WINDOW);
  tofStable = tofStableSpanMm <= TOF_STABILITY_MAX_SPAN_MM;
}

void scanI2cBusOnce() {
  Serial.println();
  Serial.println("Startup I2C scan on product bus SDA=GPIO45 SCL=GPIO46");

  uint8_t found = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
      Serial.printf("  found 0x%02X", addr);

      if (addr == VL53L1X_ADDR) {
        Serial.print("  <- VL53L1X default address");
      }

      Serial.println();
      found++;
    }
  }

  if (found == 0) {
    Serial.println("  no I2C devices found");
  }

  Serial.printf("VL53L1X presence: %s\n",
                i2cProbe(VL53L1X_ADDR) ? "PASS" : "FAIL / not seen at 0x29");
}

void setupTof() {
  Serial.println();
  Serial.println("VL53L1X setup");

  if (!i2cProbe(VL53L1X_ADDR)) {
    Serial.println("VL53L1X init skipped: not seen at 0x29");
    tofReady = false;
    return;
  }

  tof.setBus(&Wire);
  tof.setTimeout(250);

  if (!tof.init()) {
    Serial.println("VL53L1X init: FAIL");
    tofReady = false;
    return;
  }

  if (!tof.setDistanceMode(VL53L1X::Long)) {
    Serial.println("VL53L1X distance mode: FAIL");
    tofReady = false;
    return;
  }

  if (!tof.setMeasurementTimingBudget(50000)) {
    Serial.println("VL53L1X timing budget: FAIL");
    tofReady = false;
    return;
  }

  tof.startContinuous(100);
  tofReady = true;

  Serial.println("VL53L1X init: PASS");
  Serial.println("VL53L1X mode: Long");
  Serial.println("VL53L1X timing budget: 50000 us");
  Serial.println("VL53L1X continuous period: 100 ms");
  Serial.println("VL53L1X app path: only range-valid samples update lastValidTofMm");
  Serial.printf("VL53L1X stability: %u valid samples within %u mm span\n",
                TOF_STABILITY_WINDOW,
                TOF_STABILITY_MAX_SPAN_MM);
}

void reportTofIfReady() {
  if (!tofReady) {
    return;
  }

  if (!tof.dataReady()) {
    return;
  }

  uint16_t distanceMm = tof.read(false);

  if (tof.timeoutOccurred()) {
    tofTimeoutCount++;

    Serial.printf("tof ~=timeout status=timeout lastValid=%s stable=%s timeoutCount=%lu\n",
                  formatLastValidTof(),
                  formatStableTof(),
                  static_cast<unsigned long>(tofTimeoutCount));
    return;
  }

  const char* statusText =
      VL53L1X::rangeStatusToString(tof.ranging_data.range_status);

  bool rangeValid = tof.ranging_data.range_status == 0;

  if (rangeValid) {
    hasLastValidTof = true;
    lastValidTofMm = distanceMm;
    tofValidCount++;

    updateTofStability(distanceMm);

    Serial.printf("tof distance=%u mm status=%s validCount=%lu stable=%s stableMm=%s span=%u\n",
                  distanceMm,
                  statusText,
                  static_cast<unsigned long>(tofValidCount),
                  tofStableStateText(),
                  formatStableTof(),
                  tofStableSpanMm);
  } else {
    tofShadyCount++;

    Serial.printf("tof ~=%u mm status=%s lastValid=%s stable=%s shadyCount=%lu\n",
                  distanceMm,
                  statusText,
                  formatLastValidTof(),
                  formatStableTof(),
                  static_cast<unsigned long>(tofShadyCount));
  }
}

uint32_t getRawButtonIrqCount() {
  noInterrupts();
  uint32_t snapshot = buttonRawIrqCount;
  interrupts();

  return snapshot;
}

void handleButtonIrqEvent() {
  if (!buttonIrqFlag) {
    return;
  }

  noInterrupts();
  uint32_t rawIrqSnapshot = buttonRawIrqCount;
  buttonIrqFlag = false;
  interrupts();

  const unsigned long now = millis();
  const bool rawButtonState = digitalRead(PIN_BUTTON);

  if (rawButtonState != LOW) {
    return;
  }

  if (now - lastAcceptedButtonPressMs < BUTTON_DEBOUNCE_MS) {
    return;
  }

  lastAcceptedButtonPressMs = now;
  buttonPressCount++;

  Serial.printf("button event=PRESSED pressCount=%lu rawIrq=%lu\n",
                static_cast<unsigned long>(buttonPressCount),
                static_cast<unsigned long>(rawIrqSnapshot));
}

void setup() {
  delay(1500);

  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("===== FarmWhisper component validation baseline =====");

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), onButtonInterrupt, FALLING);

  pixel.begin();
  pixel.clear();
  pixel.setBrightness(32);
  setPixelStep(0);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000);

  Serial.println("GPIO42 button active LOW");
  Serial.println("GPIO42 interrupt attached on FALLING edge");
  Serial.println("GPIO42 debounced button event path enabled");
  Serial.println("GPIO41 NeoPixel color cycle enabled");
  Serial.println("Product I2C: SDA GPIO45, SCL GPIO46");

  scanI2cBusOnce();
  setupTof();
}

void loop() {
  const unsigned long now = millis();

  // Raw button state diagnostic only.
  bool rawButtonState = digitalRead(PIN_BUTTON);

  if (rawButtonState != lastRawButtonState) {
    lastRawButtonState = rawButtonState;
    Serial.printf("button raw=%s\n", rawButtonState == LOW ? "PRESSED" : "released");
  }

  // Debounced button event path for future application behavior.
  handleButtonIrqEvent();

  // NeoPixel color-cycle heartbeat.
  if (now - lastPixelMs >= 500) {
    lastPixelMs = now;
    setPixelStep(colorStep++);
  }

  // ToF distance validation read.
  if (now - lastTofReportMs >= 500) {
    lastTofReportMs = now;
    reportTofIfReady();
  }

  // Serial heartbeat.
  if (now - lastHeartbeatMs >= 2000) {
    lastHeartbeatMs = now;

    Serial.printf("heartbeat button=%s rawIrq=%lu pressCount=%lu tof=%s lastValid=%s stable=%s stableMm=%s span=%u valid=%lu shady=%lu timeout=%lu\n",
                  rawButtonState == LOW ? "PRESSED" : "released",
                  static_cast<unsigned long>(getRawButtonIrqCount()),
                  static_cast<unsigned long>(buttonPressCount),
                  tofReady ? "ready" : "not-ready",
                  formatLastValidTof(),
                  tofStableStateText(),
                  formatStableTof(),
                  tofStableSpanMm,
                  static_cast<unsigned long>(tofValidCount),
                  static_cast<unsigned long>(tofShadyCount),
                  static_cast<unsigned long>(tofTimeoutCount));
  }
}