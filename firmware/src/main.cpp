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

Adafruit_NeoPixel pixel(PIXEL_COUNT, PIN_PIXEL, NEO_GRB + NEO_KHZ800);
VL53L1X tof;

unsigned long lastPixelMs = 0;
unsigned long lastHeartbeatMs = 0;
unsigned long lastTofReportMs = 0;

uint8_t colorStep = 0;
bool lastRawButtonState = HIGH;
bool tofReady = false;

uint32_t buttonPressCount = 0;
unsigned long lastAcceptedButtonPressMs = 0;

// ISR-owned state
volatile uint32_t buttonRawIrqCount = 0;
volatile bool buttonIrqFlag = false;

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
    Serial.println("tof ~=timeout status=timeout");
    return;
  }

  const char* statusText =
      VL53L1X::rangeStatusToString(tof.ranging_data.range_status);

  bool rangeValid = tof.ranging_data.range_status == 0;

  if (rangeValid) {
    Serial.printf("tof distance=%u mm status=%s\n",
                  distanceMm,
                  statusText);
  } else {
    Serial.printf("tof ~=%u mm status=%s\n",
                  distanceMm,
                  statusText);
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

    Serial.printf("heartbeat button=%s rawIrq=%lu pressCount=%lu tof=%s\n",
                  rawButtonState == LOW ? "PRESSED" : "released",
                  static_cast<unsigned long>(getRawButtonIrqCount()),
                  static_cast<unsigned long>(buttonPressCount),
                  tofReady ? "ready" : "not-ready");
  }
}