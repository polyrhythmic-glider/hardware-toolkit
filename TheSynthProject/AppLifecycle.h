#pragma once

#include "NeoKeySequencerPanel.h"

void initApp() {
  Serial.begin(115200);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(BTN_K0, INPUT_PULLUP);

  delay(100);

  esp_reset_reason_t resetReason = esp_reset_reason();
  Serial.print("[BOOT] reset=");
  Serial.print((int)resetReason);
  Serial.print(" (");
  Serial.print(getResetReasonText(resetReason));
  Serial.println(")");
  logAudioMemory("boot");

  encoderLastState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  // Se e stabile, prova 40000000. Se vedi glitch o pixel strani, torna a 27000000.
  tft.setSPISpeed(40000000);

  tft.fillScreen(COL_BG);

  initNeoKeySequencerPanel();
  ControlSurface::begin();

  bootStartMs = millis();
  bootCompletedAtMs = 0;
  bootReady = false;
  uiMode = MODE_BOOT;
  fullRedraw = true;

  drawBootScreen();
  startDacI2s();
  startSynthAudio();

  serialDebugBegin("APPLICATION START");
  Serial.println("POLYGLIDER LCD JAM UI START");
  Serial.println("Setup complete; entering realtime loop.");
  serialDebugEnd("APPLICATION START");
}

void runAppFrame() {
  static uint32_t lastSequencerReportMs = 0;
  static int lastReportedSequencerStep = -1;
  static uint32_t lastAudioReportMs = 0;
  static uint32_t lastAudioRenderedBuffers = 0;
  static bool lastAudioTestActive = false;

  updateButtons();
  ControlSurface::tick();
  pollNeoKeySequencerKeys();
  applyBluetoothStartVolume(false);
  updateUiLogic();
  refreshNeoKeySequencerPanel();
  drawUi();

  uint32_t now = millis();
  int currentStep = sequencerCurrentStep;
  bool audioTestActive = isAudioTestActive();
  if (audioTestActive && !lastAudioTestActive) {
    Serial.print("[AUDIO][TEST] direct I2S sine wave ");
    Serial.print(AUDIO_TEST_FREQUENCY_HZ);
    Serial.print(" Hz, amplitude=");
    Serial.println(AUDIO_TEST_AMPLITUDE);
  }
  lastAudioTestActive = audioTestActive;

  if (transportRunning && currentStep != lastReportedSequencerStep &&
      now - lastSequencerReportMs >= 1000) {
    lastSequencerReportMs = now;
    lastReportedSequencerStep = currentStep;
    Serial.print("[SEQ] running step=");
    Serial.println(currentStep + 1);
  }

  uint32_t renderedBuffers = synthAudioRenderedBuffers;
  if (renderedBuffers != lastAudioRenderedBuffers && now - lastAudioReportMs >= 2000) {
    uint32_t maxRenderUs = synthAudioMaxRenderUs;
    uint32_t maxI2sUs = synthAudioMaxI2sUs;
    synthAudioMaxRenderUs = 0;
    synthAudioMaxI2sUs = 0;
    lastAudioReportMs = now;
    lastAudioRenderedBuffers = renderedBuffers;
    Serial.print("[AUDIO][RT] buffers/render/i2s/recoveries=");
    Serial.print(renderedBuffers);
    Serial.print("/");
    Serial.print(maxRenderUs);
    Serial.print("/");
    Serial.print(maxI2sUs);
    Serial.print("/");
    Serial.println(synthAudioDspRecoveries);
  }
}
