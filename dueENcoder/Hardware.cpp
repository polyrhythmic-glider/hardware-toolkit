#include "Hardware.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Encoder enc1 = {13, 23, 14, 0, 0, 0, 0, HIGH, HIGH, false, 0};
Encoder enc2 = {27, 26, 25, 0, 0, 0, 0, HIGH, HIGH, false, 0};
Encoder enc3 = {33, 32, 18, 0, 0, 0, 0, HIGH, HIGH, false, 0};

const int8_t QUAD_TABLE[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void setupDisplay() {
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
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

  enc1.lastState = readEncoderState(enc1);
  enc2.lastState = readEncoderState(enc2);
  enc3.lastState = readEncoderState(enc3);
}

void attachEncoderInterrupts() {
  attachInterrupt(digitalPinToInterrupt(enc1.pinA), readEnc1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc1.pinB), readEnc1, CHANGE);

  attachInterrupt(digitalPinToInterrupt(enc2.pinA), readEnc2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc2.pinB), readEnc2, CHANGE);

  attachInterrupt(digitalPinToInterrupt(enc3.pinA), readEnc3, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc3.pinB), readEnc3, CHANGE);
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
}

void resetEncoderValue(Encoder& enc) {
  noInterrupts();
  enc.value = 0;
  enc.accumulator = 0;
  interrupts();
}
