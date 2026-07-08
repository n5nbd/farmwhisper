#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

static constexpr int PIN_I2C_SDA = 45;
static constexpr int PIN_I2C_SCL = 46;

static constexpr int PIN_BUTTON = 42;  // active LOW
static constexpr int PIN_PIXEL  = 41;

static constexpr uint8_t PIXEL_COUNT = 1;
static constexpr uint8_t VL53L1X_ADDR = 0x29;

Adafruit_NeoPixel pixel(PIXEL_COUNT, PIN_PIXEL, NEO_GRB + NEO_KHZ800);

unsigned long lastPixelMs = 0;
unsigned long lastHeartbeatMs = 0;
unsigned long lastScanMs = 0;

uint8_t colorStep = 0;
bool lastButtonState = HIGH;

// ISR-owned state
volatile uint32_t buttonIrqCount = 0;
volatile bool buttonIrqFlag = false;

void IRAM_ATTR onButtonInterrupt() {
  buttonIrqCount++;
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

void scanI2cBus() {
  Serial.println();
  Serial.println("I2C scan on product bus SDA=GPIO45 SCL=GPIO46");

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

  if (i2cProbe(VL53L1X_ADDR)) {
    Serial.println("VL53L1X presence: PASS");
  } else {
    Serial.println("VL53L1X presence: FAIL / not seen at 0x29");
  }
}

void setup() {
  delay(1500);

  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("===== FarmWhisper product I2C smoke test =====");

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
  Serial.println("GPIO41 NeoPixel color cycle enabled");
  Serial.println("Product I2C: SDA GPIO45, SCL GPIO46");

  scanI2cBus();
}

void loop() {
  const unsigned long now = millis();

  // Normal polling path preserved.
  bool buttonState = digitalRead(PIN_BUTTON);

  if (buttonState != lastButtonState) {
    lastButtonState = buttonState;
    Serial.printf("button poll=%s\n", buttonState == LOW ? "PRESSED" : "released");
  }

  // Interrupt path preserved.
  static uint32_t lastReportedIrqCount = 0;
  static unsigned long lastAcceptedPressMs = 0;

  if (buttonIrqFlag) {
    noInterrupts();
    uint32_t irqCountSnapshot = buttonIrqCount;
    buttonIrqFlag = false;
    interrupts();

    if (irqCountSnapshot != lastReportedIrqCount && now - lastAcceptedPressMs > 50) {
      lastReportedIrqCount = irqCountSnapshot;
      lastAcceptedPressMs = now;

      Serial.printf("button interrupt press irqCount=%lu\n",
                    static_cast<unsigned long>(irqCountSnapshot));
    }
  }

  // NeoPixel heartbeat / color cycle.
  if (now - lastPixelMs >= 500) {
    lastPixelMs = now;
    setPixelStep(colorStep++);
  }

  // Serial heartbeat.
  if (now - lastHeartbeatMs >= 2000) {
    lastHeartbeatMs = now;

    uint32_t irqCountSnapshot;
    noInterrupts();
    irqCountSnapshot = buttonIrqCount;
    interrupts();

    Serial.printf("heartbeat button=%s irqCount=%lu\n",
                  buttonState == LOW ? "PRESSED" : "released",
                  static_cast<unsigned long>(irqCountSnapshot));
  }

  // I2C scan every 5 seconds.
  if (now - lastScanMs >= 5000) {
    lastScanMs = now;
    scanI2cBus();
  }
}