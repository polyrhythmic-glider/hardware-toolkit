#include "Ui.h"

#include "Audio.h"
#include "AppConfig.h"
#include "Hardware.h"
#include "Pong.h"

void handleUiLogic() {
  if (menuOpen) {
    handleMenuLogic();
  } else {
    if (enc4.clickEvent && !(currentMode == MODE_PONG && pongGameOver)) {
      openMenu();
      clickFlashUntil4 = millis() + 180;
      clearClickEvents();
      valuesChanged = true;
      return;
    }

    handleCurrentModeLogic();
  }

  clearClickEvents();
}

void openMenu() {
  menuOpen = true;
  menuSelection = currentMode;

  noInterrupts();
  menuStartValue4 = enc4.value;
  interrupts();
}

void closeMenuAndApplySelection() {
  currentMode = (UiMode)menuSelection;
  menuOpen = false;

  prepareModeTracking();
}

void prepareModeTracking() {
  noInterrupts();
  parameterStartValue1 = enc1.value;
  parameterStartValue2 = enc2.value;
  pongStartValue1 = enc1.value;
  pongStartValue2 = enc4.value;
  interrupts();

  if (currentMode == MODE_PONG) {
    if (pongGameOver) {
      resetPongGame();
    }

    lastPongUpdate = millis();
  } else if (currentMode == MODE_WAVEFORM) {
    resetAudioPhase();
  }
}

void handleMenuLogic() {
  long v4;

  noInterrupts();
  v4 = enc4.value;
  interrupts();

  int delta = v4 - menuStartValue4;

  if (delta != 0) {
    menuSelection += delta;

    while (menuSelection < 0) {
      menuSelection += MODE_COUNT;
    }

    menuSelection = menuSelection % MODE_COUNT;

    menuStartValue4 = v4;
    valuesChanged = true;
  }

  if (enc4.clickEvent) {
    closeMenuAndApplySelection();
    clickFlashUntil4 = millis() + 180;
    valuesChanged = true;
  }
}

void handleCurrentModeLogic() {
  if (currentMode == MODE_ENCODERS) {
    handleEncoderModeLogic();
  } else if (currentMode == MODE_WAVEFORM) {
    handleWaveformModeLogic();
  } else if (currentMode == MODE_PONG) {
    handlePongInputLogic();
  }
}

void handleEncoderModeLogic() {
  if (enc1.clickEvent) {
    resetEncoderValue(enc1);
    clickFlashUntil1 = millis() + 180;
    valuesChanged = true;
  }

  if (enc2.clickEvent) {
    resetEncoderValue(enc2);
    clickFlashUntil2 = millis() + 180;
    valuesChanged = true;
  }

  if (enc3.clickEvent) {
    resetEncoderValue(enc3);
    clickFlashUntil3 = millis() + 180;
    valuesChanged = true;
  }
}

void handleWaveformModeLogic() {
  long v1;
  long v2;

  noInterrupts();
  v1 = enc1.value;
  v2 = enc2.value;
  interrupts();

  int deltaA = v1 - parameterStartValue1;
  int deltaB = v2 - parameterStartValue2;

  if (deltaA != 0) {
    parameterA += deltaA;
    parameterA = constrain(parameterA, -50, 50);
    parameterStartValue1 = v1;
    valuesChanged = true;
  }

  if (deltaB != 0) {
    parameterB += deltaB;
    parameterB = constrain(parameterB, -50, 50);
    parameterStartValue2 = v2;
    valuesChanged = true;
  }

  if (enc1.clickEvent) {
    waveformIndex--;

    if (waveformIndex < WAVE_SINE) {
      waveformIndex = WAVE_CONST;
    }

    clickFlashUntil1 = millis() + 180;
    valuesChanged = true;
  }

  if (enc2.clickEvent) {
    waveformIndex++;

    if (waveformIndex > WAVE_CONST) {
      waveformIndex = WAVE_SINE;
    }

    clickFlashUntil2 = millis() + 180;
    valuesChanged = true;
  }
}
