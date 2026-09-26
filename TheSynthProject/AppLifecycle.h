#pragma once

#include "NeoKeySequencerPanel.h"

void initApp() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  initializeSequencerPitchDefaults();
  initializeDefaultModulationRouting();

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(BTN_K0, INPUT_PULLUP);

  // Lascia al bridge USB-seriale il tempo di riaprirsi dopo l'auto-reset del
  // Monitor Seriale, cosi il riepilogo non viene perso nei primissimi ms.
  delay(400);

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
  DaisyBridge::begin();
}

void runAppFrame() {
  static uint32_t lastSequencerReportMs = 0;
  static int lastReportedSequencerStep = -1;
  static bool lastAudioTestActive = false;
  static bool startupConnectionReportPrinted = false;
  static uint32_t daisyStableSinceMs = 0;

  updateButtons();
  ControlSurface::tick();
  DaisyBridge::tick();
  pollNeoKeySequencerKeys();
  updateUiLogic();
  refreshNeoKeySequencerPanel();
  drawUi();

  uint32_t now = millis();
  if (!startupConnectionReportPrinted && now - bootStartMs >= 1500) {
    startupConnectionReportPrinted = true;
    printStartupConnectionMap();
  }

  // Boot diagnostics are useful while the two boards negotiate their first
  // link. Once three acknowledged snapshots have remained stable for a short
  // period, stop all recurring USB-serial output. Manual diagnostics from
  // Utility stay enabled until the user turns them off.
  if (serialDiagnosticsAutoMuteArmed) {
    if (DaisyBridge::isConnected()) {
      if (daisyStableSinceMs == 0) daisyStableSinceMs = now;
      if (now - daisyStableSinceMs >= 2500) {
        Serial.println("[DIAG] Daisy stable; live monitor paused (Utility > Serial Diagnostics)");
        serialDiagnosticsEnabled = false;
        serialDiagnosticsAutoMuteArmed = false;
      }
    } else {
      daisyStableSinceMs = 0;
    }
  }

  int currentStep = sequencerCurrentStep;
  bool audioTestActive = isAudioTestActive();
  if (isSerialDiagnosticsEnabled() && audioTestActive && !lastAudioTestActive) {
    Serial.print("[AUDIO][TEST] Daisy sine wave ");
    Serial.print(AUDIO_TEST_FREQUENCY_HZ);
    Serial.println(" Hz");
  }
  lastAudioTestActive = audioTestActive;

  if (isSerialDiagnosticsEnabled() && transportRunning &&
      currentStep != lastReportedSequencerStep &&
      now - lastSequencerReportMs >= 750) {
    lastSequencerReportMs = now;
    lastReportedSequencerStep = currentStep;
    Serial.print("[SEQ][RT] ms=");
    Serial.print(now);
    Serial.print(" step=");
    Serial.print(currentStep + 1);
    Serial.print("/");
    Serial.print(getSequencerLength());
    Serial.print(" selected=");
    if (sequencerSelectedStep >= 0) Serial.print(sequencerSelectedStep + 1);
    else Serial.print("NONE");
    Serial.print(" track=");
    Serial.print(getSequencerTrackLabel(getActiveSequencerTrack()));
    Serial.print(" bpm=");
    Serial.print(sequencerBpm);
    Serial.print(" V1=");
    Serial.print(isSequencerStepEnabled(0, currentStep) ? "ON" : "OFF");
    Serial.print("(");
    char voice1Pitch[8];
    formatMidiPitch(getSequencerStepPitch(0, currentStep), voice1Pitch, sizeof(voice1Pitch));
    Serial.print(voice1Pitch);
    Serial.print(") V2=");
    Serial.print(isSequencerStepEnabled(1, currentStep) ? "ON" : "OFF");
    Serial.print("(");
    char voice2Pitch[8];
    formatMidiPitch(getSequencerStepPitch(1, currentStep), voice2Pitch, sizeof(voice2Pitch));
    Serial.print(voice2Pitch);
    Serial.print(") K=");
    Serial.println(isKickStepEnabled(currentStep) ? "HIT" : "REST");
  }

}
