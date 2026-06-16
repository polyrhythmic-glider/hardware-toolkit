#include "Hardware.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t activePca9548aAddress = PCA9548A_ADDRESS;
uint8_t activeOledAddress = OLED_ADDRESS;
bool pca9548aFound = false;
bool displayHardwareReady = false;

Encoder enc1 = {13, 23, 14, 0, 0, 0, 0, HIGH, HIGH, false, 0};
Encoder enc2 = {27, 26, 25, 0, 0, 0, 0, HIGH, HIGH, false, 0};
Encoder enc3 = {33, 32, 18, 0, 0, 0, 0, HIGH, HIGH, false, 0};
Encoder enc4 = {19, 16, 17, 0, 0, 0, 0, HIGH, HIGH, false, 0};

const int8_t QUAD_TABLE[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void setupDisplay() {
  pinMode(OLED_SDA, INPUT_PULLUP);
  pinMode(OLED_SCL, INPUT_PULLUP);
  delay(10);

  if (DEBUG_SERIAL) {
    Serial.print(F("Pre-Wire pin levels: SDA="));
    Serial.print(digitalRead(OLED_SDA));
    Serial.print(F(" SCL="));
    Serial.println(digitalRead(OLED_SCL));
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(I2C_CLOCK_HZ);

  if (DIRECT_OLED_TEST) {
    setupDirectOledTest();
    return;
  }

  findPca9548aAddress();

  if (DEBUG_SERIAL) {
    Serial.print(F("I2C SDA GPIO "));
    Serial.print(OLED_SDA);
    Serial.print(F(", SCL GPIO "));
    Serial.print(OLED_SCL);
    Serial.print(F(", clock "));
    Serial.print(I2C_CLOCK_HZ);
    Serial.println(F(" Hz"));
    printI2cDiagnostics();
  }

  if (!pca9548aFound) {
    bool directOledPresent = isI2cDevicePresent(OLED_ADDRESS);

    if (DEBUG_SERIAL) {
      Serial.print(F("No PCA9548A detected. Direct root OLED 0x"));
      Serial.print(OLED_ADDRESS, HEX);
      Serial.println(directOledPresent ? F(" ACK") : F(" NO ACK"));
      Serial.println(F("Skipping mux channel OLED init until the PCA is visible."));
    }

    if (directOledPresent) {
      display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
      displayHardwareReady = true;
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print(F("DIRECT OLED"));
      display.setCursor(0, 16);
      display.print(F("NO PCA9548A"));
      display.setCursor(0, 32);
      display.print(F("Check wiring"));
      display.display();
    }

    return;
  }

  displayHardwareReady = true;

  for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
    selectDisplay(i);
    delay(10);
    bool oledPresent = isI2cDevicePresent(OLED_ADDRESS);
    bool displayStarted = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);

    if (DEBUG_SERIAL) {
      Serial.print(F("OLED channel "));
      Serial.print(i);
      Serial.print(F(": address 0x"));
      Serial.print(OLED_ADDRESS, HEX);
      Serial.print(oledPresent ? F(" ACK") : F(" NO ACK"));
      Serial.print(F(", begin="));
      Serial.println(displayStarted ? F("OK") : F("FAIL"));
    }

    drawPcaChannelTestScreen(i, oledPresent, displayStarted);
  }

  selectDisplay(0);
}

void drawPcaChannelTestScreen(uint8_t channel, bool oledPresent, bool displayStarted) {
  display.clearDisplay();
  display.display();
  delay(80);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("OLED TEST"));
  display.print(F("CH "));
  display.println(channel + 1);
  display.print(F("SIZE "));
  display.print(SCREEN_WIDTH);
  display.print(F("x"));
  display.println(SCREEN_HEIGHT);
  display.print(F("ADDR 0x"));
  display.println(OLED_ADDRESS, HEX);
  display.println(oledPresent ? F("ACK OK") : F("NO ACK"));
  display.println(displayStarted ? F("BEGIN OK") : F("BEGIN FAIL"));

  display.display();
  delay(150);
}

void selectPca9548aChannel(uint8_t channel) {
  if (channel > 7) {
    return;
  }

  Wire.beginTransmission(activePca9548aAddress);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(5);
}

void disablePca9548aChannels() {
  for (uint8_t address = 0x70; address <= 0x77; address++) {
    Wire.beginTransmission(address);
    Wire.write(0);
    Wire.endTransmission();
  }
}

void setupDirectOledTest() {
  if (DEBUG_SERIAL) {
    Serial.println(F("DIRECT_OLED_TEST enabled: skipping PCA9548A and scanning only GPIO21/GPIO22 root bus."));
    printActiveI2cScan("ROOT");
  }

  if (isI2cDevicePresent(0x3C)) {
    activeOledAddress = 0x3C;
  } else if (isI2cDevicePresent(0x3D)) {
    activeOledAddress = 0x3D;
  } else {
    if (DEBUG_SERIAL) {
      Serial.println(F("No OLED ACK at 0x3C or 0x3D on GPIO21/GPIO22."));
      Serial.println(F("The ESP32 sees no I2C device. Check OLED VCC/GND/SDA/SCL pin order and solder joints."));
    }

    displayHardwareReady = false;
    return;
  }

  bool displayStarted = display.begin(SSD1306_SWITCHCAPVCC, activeOledAddress);
  displayHardwareReady = displayStarted;

  if (DEBUG_SERIAL) {
    Serial.print(F("Direct OLED found at 0x"));
    Serial.print(activeOledAddress, HEX);
    Serial.print(F(", begin="));
    Serial.println(displayStarted ? F("OK") : F("FAIL"));
  }

  if (!displayStarted) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("DIRECT OLED OK"));
  display.setCursor(0, 12);
  display.print(F("ADDR 0x"));
  display.print(activeOledAddress, HEX);
  display.drawRect(0, 24, SCREEN_WIDTH, SCREEN_HEIGHT - 24, SSD1306_WHITE);
  display.drawLine(0, 24, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, SSD1306_WHITE);
  display.drawLine(SCREEN_WIDTH - 1, 24, 0, SCREEN_HEIGHT - 1, SSD1306_WHITE);
  display.display();
}

void selectDisplay(uint8_t displayIndex) {
  if (displayIndex >= DISPLAY_COUNT) {
    displayIndex = 0;
  }

  selectPca9548aChannel(displayIndex);
}

bool findPca9548aAddress() {
  for (uint8_t address = 0x70; address <= 0x77; address++) {
    if (isI2cDevicePresent(address)) {
      activePca9548aAddress = address;
      pca9548aFound = true;
      return true;
    }
  }

  activePca9548aAddress = PCA9548A_ADDRESS;
  pca9548aFound = false;
  return false;
}

bool isI2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void printI2cDiagnostics() {
  Serial.println(F("I2C scan root bus, mux channels disabled:"));
  disablePca9548aChannels();
  printActiveI2cScan("ROOT");

  Serial.print(F("PCA9548A address scan 0x70..0x77:"));

  bool foundCandidate = false;

  for (uint8_t address = 0x70; address <= 0x77; address++) {
    if (isI2cDevicePresent(address)) {
      Serial.print(F(" 0x"));
      Serial.print(address, HEX);
      foundCandidate = true;
    }
  }

  if (!foundCandidate) {
    Serial.print(F(" none"));
  }

  Serial.println();

  if (!findPca9548aAddress()) {
    Serial.print(F("PCA9548A not found in 0x70..0x77. Preferred address was 0x"));
    Serial.println(PCA9548A_ADDRESS, HEX);
    return;
  }

  Serial.print(F("Using PCA9548A at 0x"));
  Serial.println(activePca9548aAddress, HEX);
  Serial.println(F("Scanning channels 0..3:"));

  for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
    selectDisplay(i);
    Serial.print(F("CH"));
    Serial.print(i);
    printActiveI2cScan("");
  }

  disablePca9548aChannels();
}

void printActiveI2cScan(const char* label) {
  bool foundAny = false;

  if (label[0] != '\0') {
    Serial.print(label);
    Serial.print(F(":"));
  }

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print(F(" 0x"));
      if (address < 16) {
        Serial.print(F("0"));
      }
      Serial.print(address, HEX);
      foundAny = true;
    }
  }

  if (!foundAny) {
    Serial.print(F(" none"));
  }

  Serial.println();
}

void IRAM_ATTR updateEncoder(Encoder* enc) {
  unsigned long now = micros();

  if (ENCODER_MIN_INTERVAL_US > 0) {
    if (now - enc->lastMicros < ENCODER_MIN_INTERVAL_US) {
      return;
    }
  }

  enc->lastMicros = now;

  uint8_t a = digitalRead(enc->pinA);
  uint8_t b = digitalRead(enc->pinB);

  uint8_t newState = (a << 1) | b;
  uint8_t index = (enc->lastState << 2) | newState;

  int8_t movement = QUAD_TABLE[index];

  if (movement != 0) {
    enc->accumulator += movement;

    if (enc->accumulator >= STEPS_PER_DETENT) {
      enc->value++;
      enc->accumulator = 0;
      valuesChanged = true;
    } else if (enc->accumulator <= -STEPS_PER_DETENT) {
      enc->value--;
      enc->accumulator = 0;
      valuesChanged = true;
    }
  }

  enc->lastState = newState;
}

void IRAM_ATTR readEnc1() {
  updateEncoder(&enc1);
}

void IRAM_ATTR readEnc2() {
  updateEncoder(&enc2);
}

void IRAM_ATTR readEnc3() {
  updateEncoder(&enc3);
}

void IRAM_ATTR readEnc4() {
  updateEncoder(&enc4);
}

void setupEncoderPins(Encoder& enc) {
  pinMode(enc.pinA, INPUT_PULLUP);
  pinMode(enc.pinB, INPUT_PULLUP);
  pinMode(enc.pinSW, INPUT_PULLUP);

  enc.lastButton = digitalRead(enc.pinSW);
  enc.currentButton = enc.lastButton;
  enc.lastButtonChange = millis();
}

void setupAllEncoders() {
  setupEncoderPins(enc1);
  setupEncoderPins(enc2);
  setupEncoderPins(enc3);
  setupEncoderPins(enc4);

  enc1.lastState = readEncoderState(enc1);
  enc2.lastState = readEncoderState(enc2);
  enc3.lastState = readEncoderState(enc3);
  enc4.lastState = readEncoderState(enc4);
}

void attachEncoderInterrupts() {
  attachInterrupt(digitalPinToInterrupt(enc1.pinA), readEnc1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc1.pinB), readEnc1, CHANGE);

  attachInterrupt(digitalPinToInterrupt(enc2.pinA), readEnc2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc2.pinB), readEnc2, CHANGE);

  attachInterrupt(digitalPinToInterrupt(enc3.pinA), readEnc3, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc3.pinB), readEnc3, CHANGE);

  attachInterrupt(digitalPinToInterrupt(enc4.pinA), readEnc4, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc4.pinB), readEnc4, CHANGE);
}

uint8_t readEncoderState(Encoder& enc) {
  uint8_t a = digitalRead(enc.pinA);
  uint8_t b = digitalRead(enc.pinB);

  return (a << 1) | b;
}

void readButtons() {
  readSingleButton(enc1);
  readSingleButton(enc2);
  readSingleButton(enc3);
  readSingleButton(enc4);
}

void readSingleButton(Encoder& enc) {
  int raw = digitalRead(enc.pinSW);
  unsigned long now = millis();

  if (raw != enc.lastButton && now - enc.lastButtonChange > BUTTON_DEBOUNCE_MS) {
    enc.lastButtonChange = now;
    enc.lastButton = raw;
    enc.currentButton = raw;

    if (raw == LOW) {
      enc.clickEvent = true;
    }
  }
}

void clearClickEvents() {
  enc1.clickEvent = false;
  enc2.clickEvent = false;
  enc3.clickEvent = false;
  enc4.clickEvent = false;
}

void resetEncoderValue(Encoder& enc) {
  noInterrupts();
  enc.value = 0;
  enc.accumulator = 0;
  interrupts();
}
