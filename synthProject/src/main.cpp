#include "AppConfig.h"
#include "Hardware.h"
#include "Pong.h"
#include "Render.h"
#include "Ui.h"

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
  Serial.begin(115200);
  Serial.println("Encoder controller starting");

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
