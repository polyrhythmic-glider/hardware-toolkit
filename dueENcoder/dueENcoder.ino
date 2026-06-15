#include "AppConfig.h"
#include "Hardware.h"
#include "Pong.h"
#include "Render.h"
#include "Ui.h"

// ======================================================
//  POLYRHYTHMIC GLIDER - 3 ENCODER UI + PONG
//  Board: ESP32 DevKit V1 / ESP32-WROOM-32
//  Display: OLED SSD1306 128x64 I2C
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
//  ENCODER 3 - MENU COMMANDER:
//    A  -> GPIO33 / D33
//    B  -> GPIO32 / D32
//    SW -> GPIO18 / D18
//
//  OLED:
//    SDA -> GPIO21 / D21
//    SCL -> GPIO22 / D22
// ======================================================

volatile bool valuesChanged = true;

unsigned long clickFlashUntil1 = 0;
unsigned long clickFlashUntil2 = 0;
unsigned long clickFlashUntil3 = 0;

UiMode currentMode = MODE_ENCODERS;

bool menuOpen = true;
int menuSelection = 0;
long menuStartValue3 = 0;

long parameterStartValue1 = 0;
long parameterStartValue2 = 0;

int waveformIndex = WAVE_SINE;
int parameterA = 0;
int parameterB = 0;

void setup() {
  setupDisplay();

  setupAllEncoders();
  attachEncoderInterrupts();

  menuSelection = currentMode;

  noInterrupts();
  menuStartValue3 = enc3.value;
  parameterStartValue1 = enc1.value;
  parameterStartValue2 = enc2.value;
  pongStartValue1 = enc1.value;
  pongStartValue2 = enc2.value;
  interrupts();

  resetPongGame();

  drawScreen();
}

void loop() {
  readButtons();
  handleUiLogic();

  static unsigned long lastDraw = 0;
  static bool wasFlashing = false;

  bool isFlashing =
    millis() < clickFlashUntil1 ||
    millis() < clickFlashUntil2 ||
    millis() < clickFlashUntil3;

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
