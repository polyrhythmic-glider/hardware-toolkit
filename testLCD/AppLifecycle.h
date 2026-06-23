#pragma once

void initApp() {
  Serial.begin(115200);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(BTN_K0, INPUT_PULLUP);

  delay(100);

  encoderLastState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  // Se e stabile, prova 40000000. Se vedi glitch o pixel strani, torna a 27000000.
  tft.setSPISpeed(40000000);

  tft.fillScreen(COL_BG);

  bootStartMs = millis();
  bootCompletedAtMs = 0;
  bootReady = false;
  uiMode = MODE_BOOT;
  fullRedraw = true;

  drawBootScreen();
  startDacI2s();
  startSynthAudio();

  Serial.println("POLYGLIDER LCD JAM UI START");
}

void runAppFrame() {
  updateButtons();
  applyBluetoothStartVolume(false);
  updateUiLogic();
  drawUi();
}
