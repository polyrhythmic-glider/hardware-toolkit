#pragma once

#include "NeoKeySequencerPanel.h"

void initApp() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  initializeSequencerPitchDefaults();
  initializeDefaultModulationRouting();

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(BTN_K0, INPUT_PULLUP);

  // Lascia al bridge USB-seriale il tempo di riaprirsi dopo l'auto-reset del
  // Monitor Seriale, cosi il riepilogo non viene perso nei primissimi ms.
  delay(400);

  if (isSerialDiagnosticsEnabled()) {
    Serial.println();
    Serial.println();
    serialDebugBegin("ESP32 BOOT CONNECTION CHECK");
    Serial.println("[BOOT][1/6] Serial monitor READY - 115200 baud");
    esp_reset_reason_t resetReason = esp_reset_reason();
    Serial.print("[BOOT] reset=");
    Serial.print((int)resetReason);
    Serial.print(" (");
    Serial.print(getResetReasonText(resetReason));
    Serial.println(")");
    logAudioMemory("boot");
    serialDebugEnd("ESP32 BOOT CONNECTION CHECK");
  }

  encoderLastState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  // Se e stabile, prova 40000000. Se vedi glitch o pixel strani, torna a 27000000.
  tft.setSPISpeed(40000000);

  tft.fillScreen(COL_BG);

  if (isSerialDiagnosticsEnabled()) {
    Serial.println("[BOOT][2/6] TFT ST7735 SPI READY");
  }

  initNeoKeySequencerPanel();
  if (isSerialDiagnosticsEnabled()) {
    Serial.print("[BOOT][3/6] NeoKey READY ");
    Serial.print(getReadyNeoKeyCount());
    Serial.println("/4");
  }

  ControlSurface::begin();
  if (isSerialDiagnosticsEnabled()) {
    Serial.print("[BOOT][4/6] PCA/OLED/MCP scan complete - OLED ");
    Serial.print(countReadyBits(ControlSurface::getOledReadyMask()));
    Serial.print("/4 MCP ");
    Serial.println(ControlSurface::isEncoderExpanderReady() ? "READY" : "MISSING");
  }

  bootStartMs = millis();
  bootCompletedAtMs = 0;
  bootReady = false;
  uiMode = MODE_BOOT;
  fullRedraw = true;

  drawBootScreen();
#if SYNTH_AUDIO_BACKEND_DAISY
  DaisyBridge::begin();
  if (isSerialDiagnosticsEnabled()) {
    Serial.println("[BOOT][5/6] Daisy UART started - waiting for bidirectional link");
  }
#else
  startSynthAudio();
  if (isSerialDiagnosticsEnabled()) {
    Serial.println("[BOOT][5/6] PCM5102 I2S backend initialized");
  }
#endif

  if (isSerialDiagnosticsEnabled()) {
    serialDebugBegin("APPLICATION START");
    Serial.println("POLYGLIDER LCD JAM UI START");
    Serial.println("Setup complete; entering realtime loop.");
    serialDebugEnd("APPLICATION START");
  }
}

void runAppFrame() {
  static uint32_t lastSequencerReportMs = 0;
  static int lastReportedSequencerStep = -1;
#if !SYNTH_AUDIO_BACKEND_DAISY
  static uint32_t lastAudioReportMs = 0;
  static uint32_t lastAudioRenderedBuffers = 0;
#endif
  static bool lastAudioTestActive = false;
  static bool startupConnectionReportPrinted = false;

  updateButtons();
  ControlSurface::tick();
  pollNeoKeySequencerKeys();
  updateUiLogic();
#if SYNTH_AUDIO_BACKEND_DAISY
  DaisyBridge::tick();
#endif
  refreshNeoKeySequencerPanel();
  drawUi();

  uint32_t now = millis();
  if (isSerialDiagnosticsEnabled() && !startupConnectionReportPrinted &&
      now - bootStartMs >= 1000) {
    startupConnectionReportPrinted = true;
    Serial.println("[BOOT][6/6] Final connection summary");
    printConnectionDiagnosticsSnapshot();
  }

  int currentStep = sequencerCurrentStep;
  bool audioTestActive = isAudioTestActive();
  if (isSerialDiagnosticsEnabled() && audioTestActive && !lastAudioTestActive) {
#if SYNTH_AUDIO_BACKEND_DAISY
    Serial.print("[AUDIO][TEST] Daisy sine wave ");
#else
    Serial.print("[AUDIO][TEST] direct I2S sine wave ");
#endif
    Serial.print(AUDIO_TEST_FREQUENCY_HZ);
    Serial.print(" Hz, amplitude=");
    Serial.println(AUDIO_TEST_AMPLITUDE);
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
    Serial.print(sequencerSelectedStep + 1);
    Serial.print(" track=");
    Serial.print(getSequencerTrackLabel(getActiveSequencerTrack()));
    Serial.print(" bpm=");
    Serial.print(sequencerBpm);
#if !SYNTH_AUDIO_BACKEND_DAISY
    Serial.print(" clock=");
    Serial.print((uint32_t)sequencerSampleCounter);
    Serial.print("/");
    Serial.print(getSequencerStepSamples());
    Serial.print(" gate=");
    Serial.print(getSequencerGateSamples());
#endif
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

#if !SYNTH_AUDIO_BACKEND_DAISY
  uint32_t renderedBuffers = synthAudioRenderedBuffers;
  if (isSerialDiagnosticsEnabled() && renderedBuffers != lastAudioRenderedBuffers &&
      now - lastAudioReportMs >= 2000) {
    uint32_t maxRenderUs = synthAudioMaxRenderUs;
    uint32_t maxI2sUs = synthAudioMaxI2sUs;
    synthAudioMaxRenderUs = 0;
    synthAudioMaxI2sUs = 0;
    lastAudioReportMs = now;
    lastAudioRenderedBuffers = renderedBuffers;
    Serial.print("[AUDIO][RT] buffers/render_us/i2s_us/short/recoveries/free_heap/stack=");
    Serial.print(renderedBuffers);
    Serial.print("/");
    Serial.print(maxRenderUs);
    Serial.print("/");
    Serial.print(maxI2sUs);
    Serial.print("/");
    Serial.print(synthAudioShortWrites);
    Serial.print("/");
    Serial.print(synthAudioDspRecoveries);
    Serial.print("/");
    Serial.print(ESP.getFreeHeap());
    Serial.print("/");
    Serial.println(synthAudioTaskHandle != nullptr
                     ? uxTaskGetStackHighWaterMark(synthAudioTaskHandle)
                     : 0);
  }
#endif
}
