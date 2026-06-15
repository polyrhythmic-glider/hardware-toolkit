#include "AppConfig.h"
#include "Hardware.h"
#include "Pong.h"
#include "Render.h"
#include "Ui.h"

// ======================================================
//  POLYRHYTHMIC GLIDER - 4 ENCODER / 4 OLED UI + PONG
//  Board: ESP32 DevKit V1 / ESP32-WROOM-32
//  Mux: PCA9548A I2C address 0x70
//  Displays: 4x OLED SSD1306 128x64 I2C address 0x3C
//
//  START:
//    Parte dal MENU.
//
//  MODES:
//    0 = Encoder monitor con tacchette
//    1 = Waveform editor
//    2 = Mini Pong
//
//  ENCODER 1:
//    A  -> GPIO13 / D13
//    B  -> GPIO23 / D23
//    SW -> GPIO14 / D14
//
//  ENCODER 2:
//    A  -> GPIO27 / D27
//    B  -> GPIO26 / D26
//    SW -> GPIO25 / D25
//
//  ENCODER 3:
//    A  -> GPIO33 / D33
//    B  -> GPIO32 / D32
//    SW -> GPIO18 / D18
//
//  ENCODER 4 - MENU COMMANDER:
//    A  -> GPIO19 / D19
//    B  -> GPIO16 / RX2
//    SW -> GPIO17 / TX2
//
//  PCA9548A:
//    SDA -> GPIO21 / D21
//    SCL -> GPIO22 / D22
//    OLED 1..4 -> channels 0..3
// ======================================================

volatile bool valuesChanged = true;
bool debugMode = DEBUG_SERIAL;
bool serialDebugStarted = false;

unsigned long clickFlashUntil1 = 0;
unsigned long clickFlashUntil2 = 0;
unsigned long clickFlashUntil3 = 0;
unsigned long clickFlashUntil4 = 0;

UiMode currentMode = MODE_ENCODERS;

bool menuOpen = true;
int menuSelection = 0;
long menuStartValue4 = 0;

long parameterStartValue1 = 0;
long parameterStartValue2 = 0;

int waveformIndex = WAVE_SINE;
int parameterA = 0;
int parameterB = 0;

void setupSerialDebug();
bool handleDebugToggle();
void setDebugMode(bool enabled);
void printEncoderDebug(const char* label);
void printDebugHeartbeat();

void setup() {
  setupSerialDebug();

  setupDisplay();

  setupAllEncoders();
  printEncoderDebug("encoder pins ready");

  attachEncoderInterrupts();

  menuSelection = currentMode;

  noInterrupts();
  menuStartValue4 = enc4.value;
  parameterStartValue1 = enc1.value;
  parameterStartValue2 = enc2.value;
  pongStartValue1 = enc1.value;
  pongStartValue2 = enc4.value;
  interrupts();

  resetPongGame();

  drawScreen();

  if (debugMode) {
    Serial.println(F("Setup complete. Open Serial Monitor at 115200 baud."));
    if (DIRECT_OLED_TEST) {
      Serial.println(F("Expected direct OLED test: ROOT should show 0x3C or 0x3D."));
    } else if (PCA_MUX_TEST) {
      Serial.println(F("Expected PCA test: ROOT should show 0x70, channels should show 0x3C."));
    } else {
      Serial.println(F("Expected I2C: ROOT has 0x70, each CH0..CH3 has 0x70 and 0x3C."));
    }
    Serial.println(F("Turn/push encoders: values and click flags should change below."));
  }
}

void loop() {
  readButtons();

  bool debugComboHeld = handleDebugToggle();

  if (debugComboHeld) {
    clearClickEvents();
  }

  printDebugHeartbeat();

  if (!debugComboHeld) {
    handleUiLogic();
  }

  static unsigned long lastDraw = 0;
  static bool wasFlashing = false;

  bool isFlashing =
    millis() < clickFlashUntil1 ||
    millis() < clickFlashUntil2 ||
    millis() < clickFlashUntil3 ||
    millis() < clickFlashUntil4;

  if (isFlashing || wasFlashing) {
    valuesChanged = true;
  }

  wasFlashing = isFlashing;

  if (currentMode == MODE_PONG && !menuOpen) {
    updatePong();
  }

  if (valuesChanged && millis() - lastDraw >= 35) {
    valuesChanged = false;
    lastDraw = millis();
    drawScreen();
  }
}

void setupSerialDebug() {
  if (!debugMode) {
    return;
  }

  Serial.begin(DEBUG_SERIAL_BAUD);
  serialDebugStarted = true;
  delay(800);
  Serial.println();
  Serial.println(F("==== dueENcoder debug boot ===="));
}

bool handleDebugToggle() {
  static unsigned long holdStartedAt = 0;
  static bool toggleConsumed = false;

  bool comboHeld = digitalRead(enc1.pinSW) == LOW && digitalRead(enc4.pinSW) == LOW;

  if (!comboHeld) {
    holdStartedAt = 0;
    toggleConsumed = false;
    return false;
  }

  if (holdStartedAt == 0) {
    holdStartedAt = millis();
  }

  if (!toggleConsumed && millis() - holdStartedAt >= DEBUG_TOGGLE_HOLD_MS) {
    toggleConsumed = true;
    setDebugMode(!debugMode);
  }

  return true;
}

void setDebugMode(bool enabled) {
  if (debugMode == enabled) {
    return;
  }

  if (enabled && !serialDebugStarted) {
    Serial.begin(DEBUG_SERIAL_BAUD);
    serialDebugStarted = true;
    delay(50);
  }

  debugMode = enabled;

  if (serialDebugStarted) {
    Serial.println();
    Serial.println(debugMode ? F("==== DEBUG MODE ON ====") : F("==== DEBUG MODE OFF ===="));
  }

  clickFlashUntil1 = millis() + 250;
  clickFlashUntil4 = millis() + 250;
  valuesChanged = true;
}

void printEncoderDebug(const char* label) {
  if (!debugMode) {
    return;
  }

  long v1;
  long v2;
  long v3;
  long v4;

  noInterrupts();
  v1 = enc1.value;
  v2 = enc2.value;
  v3 = enc3.value;
  v4 = enc4.value;
  interrupts();

  Serial.print(F("["));
  Serial.print(millis());
  Serial.print(F(" ms] "));
  Serial.print(label);
  Serial.print(F(" | mode="));
  Serial.print((int)currentMode);
  Serial.print(F(" menu="));
  Serial.print(menuOpen ? F("open") : F("closed"));

  Serial.print(F(" | E1="));
  Serial.print(v1);
  Serial.print(F(" A"));
  Serial.print(digitalRead(enc1.pinA));
  Serial.print(F(" B"));
  Serial.print(digitalRead(enc1.pinB));
  Serial.print(F(" SW"));
  Serial.print(digitalRead(enc1.pinSW));
  Serial.print(enc1.clickEvent ? F(" CLICK") : F(""));

  Serial.print(F(" | E2="));
  Serial.print(v2);
  Serial.print(F(" A"));
  Serial.print(digitalRead(enc2.pinA));
  Serial.print(F(" B"));
  Serial.print(digitalRead(enc2.pinB));
  Serial.print(F(" SW"));
  Serial.print(digitalRead(enc2.pinSW));
  Serial.print(enc2.clickEvent ? F(" CLICK") : F(""));

  Serial.print(F(" | E3="));
  Serial.print(v3);
  Serial.print(F(" A"));
  Serial.print(digitalRead(enc3.pinA));
  Serial.print(F(" B"));
  Serial.print(digitalRead(enc3.pinB));
  Serial.print(F(" SW"));
  Serial.print(digitalRead(enc3.pinSW));
  Serial.print(enc3.clickEvent ? F(" CLICK") : F(""));

  Serial.print(F(" | E4="));
  Serial.print(v4);
  Serial.print(F(" A"));
  Serial.print(digitalRead(enc4.pinA));
  Serial.print(F(" B"));
  Serial.print(digitalRead(enc4.pinB));
  Serial.print(F(" SW"));
  Serial.print(digitalRead(enc4.pinSW));
  Serial.print(enc4.clickEvent ? F(" CLICK") : F(""));

  Serial.println();
}

void printDebugHeartbeat() {
  if (!debugMode) {
    return;
  }

  static unsigned long lastPrint = 0;
  static long lastV1 = 0;
  static long lastV2 = 0;
  static long lastV3 = 0;
  static long lastV4 = 0;

  long v1;
  long v2;
  long v3;
  long v4;

  noInterrupts();
  v1 = enc1.value;
  v2 = enc2.value;
  v3 = enc3.value;
  v4 = enc4.value;
  interrupts();

  bool encoderChanged = v1 != lastV1 || v2 != lastV2 || v3 != lastV3 || v4 != lastV4;
  bool clickDetected = enc1.clickEvent || enc2.clickEvent || enc3.clickEvent || enc4.clickEvent;
  bool heartbeatDue = millis() - lastPrint >= DEBUG_HEARTBEAT_INTERVAL_MS;

  if (!encoderChanged && !clickDetected && !heartbeatDue) {
    return;
  }

  lastV1 = v1;
  lastV2 = v2;
  lastV3 = v3;
  lastV4 = v4;
  lastPrint = millis();

  printEncoderDebug(encoderChanged ? "encoder change" : (clickDetected ? "button click" : "heartbeat"));
}
