#pragma once

void drawBluetoothHeaderField(bool force) {
  uint32_t now = millis();

  if (!force && now - lastBtHeaderRefreshMs < BT_HEADER_REFRESH_MS) {
    return;
  }

  updateBluetoothRssi();

  bool connected = bluetoothEnabled && btAudioStarted && a2dpSink.is_connected();
  bool streaming = isBluetoothStreaming();
  int bars = getBluetoothSignalBars();
  uint16_t statusColor = streaming ? COL_GOLD : connected ? COL_NOTE : COL_MUTED;

  tft.fillRect(84, 3, 74, 14, COL_HEADER);

  tft.setTextSize(1);
  tft.setTextColor(statusColor);
  tft.setCursor(87, 6);
  tft.print("BT");

  tft.setTextColor(connected ? COL_INK : COL_MUTED);
  tft.setCursor(103, 6);
  tft.print(getBluetoothStatusText());

  for (int i = 0; i < 4; i++) {
    int h = 3 + i * 2;
    int x = 137 + i * 5;
    int y = 14 - h;
    uint16_t barColor = i < bars ? statusColor : rgb565(48, 54, 69);

    tft.drawRect(x, 5, 3, 10, rgb565(34, 40, 53));
    if (i < bars) {
      tft.fillRect(x + 1, y, 1, h, barColor);
    }
  }

  lastBtHeaderRefreshMs = now;
}

void drawHeader(const char* title) {
  tft.fillRect(0, 0, SCREEN_W, 20, COL_HEADER);
  tft.fillTriangle(0, 0, 18, 0, 0, 20, COL_VIOLET);
  tft.drawFastHLine(0, 19, SCREEN_W, COL_CORAL);

  tft.setTextSize(1);
  tft.setTextColor(COL_INK);
  tft.setCursor(7, 6);
  tft.print(title);

  lastBtHeaderRefreshMs = 0;
  drawBluetoothHeaderField(true);
}

void drawStageBackdrop() {
  tft.fillScreen(COL_BG);

  for (int y = 25; y < SCREEN_H; y += 12) {
    tft.drawFastHLine(0, y, SCREEN_W, COL_LINE);
  }

  for (int i = 0; i < 10; i++) {
    int x = 13 + i * 15;
    int y = 31 + (i * 17) % 72;
    tft.drawPixel(x, y, COL_MUTED);
    tft.drawPixel(x + 1, y, COL_MUTED);
  }
}

void drawParamIcon(int itemIndex, int cx, int cy, uint16_t color) {
  MenuAction action = menuItems[itemIndex].action;

  switch (action) {
    case ACTION_WAVE_SINE_MENU:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[0], (float)waveVoiceFold[0], (float)waveVoicePhase[0], color);
      tft.drawPixel(cx + 9, cy, COL_GOLD);
      break;

    case ACTION_WAVE_SQUARE_MENU:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[1], (float)waveVoiceFold[1], (float)waveVoicePhase[1], color);
      tft.drawPixel(cx + 9, cy, COL_GOLD);
      break;

    case ACTION_WAVE_TRIANGLE_MENU:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[2], (float)waveVoiceFold[2], (float)waveVoicePhase[2], color);
      tft.drawPixel(cx + 9, cy, COL_GOLD);
      break;

    case ACTION_WAVE_SAW_MENU:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[3], (float)waveVoiceFold[3], (float)waveVoicePhase[3], color);
      tft.drawPixel(cx + 9, cy, COL_GOLD);
      break;

    case ACTION_FILTER_MENU:
      tft.drawFastHLine(cx - 8, cy + 5, 16, rgb565(62, 68, 82));
      tft.drawLine(cx - 8, cy - 5, cx, cy - 1, color);
      tft.drawLine(cx, cy - 1, cx + 8, cy + 5, color);
      tft.drawPixel(cx + 10, cy, COL_GOLD);
      break;

    case ACTION_KICK_MENU:
      tft.drawCircle(cx, cy, 8, color);
      tft.fillCircle(cx - 3, cy + 2, 4, COL_GOLD);
      tft.drawLine(cx - 9, cy + 8, cx + 9, cy + 8, color);
      break;

    case ACTION_VOICE_WAVE:
    {
      int voice = getCurrentWaveVoiceIndex();
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[voice], (float)waveVoiceFold[voice], (float)waveVoicePhase[voice], color);
      tft.drawPixel(cx + 9, cy, COL_GOLD);
      break;
    }

    case ACTION_VOICE_TUNE:
      tft.drawFastVLine(cx, cy - 7, 15, color);
      tft.drawLine(cx - 6, cy - 5, cx, cy - 1, color);
      tft.drawLine(cx + 6, cy - 5, cx, cy - 1, color);
      tft.fillCircle(cx, cy + 5, 2, COL_GOLD);
      break;

    case ACTION_VOICE_FOLD:
      tft.drawLine(cx - 8, cy + 4, cx - 3, cy - 5, color);
      tft.drawLine(cx - 3, cy - 5, cx + 2, cy + 5, color);
      tft.drawLine(cx + 2, cy + 5, cx + 7, cy - 4, color);
      tft.fillCircle(cx + 8, cy - 4, 1, COL_GOLD);
      break;

    case ACTION_VOICE_PHASE:
      tft.drawCircle(cx, cy, 6, color);
      tft.drawLine(cx, cy, cx + 6, cy - 1, color);
      tft.fillCircle(cx + 6, cy - 1, 2, COL_GOLD);
      break;

    case ACTION_VOICE_SCOPE:
    {
      int voice = getCurrentWaveVoiceIndex();
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[voice], (float)waveVoiceFold[voice], (float)waveVoicePhase[voice], color);
      tft.drawFastHLine(cx - 8, cy + 7, 16, COL_GOLD);
      break;
    }

    case ACTION_PITCH:
      tft.drawFastVLine(cx, cy - 6, 13, color);
      tft.drawLine(cx - 5, cy - 5, cx, cy - 1, color);
      tft.drawLine(cx + 5, cy - 5, cx, cy - 1, color);
      tft.fillCircle(cx, cy + 5, 2, color);
      break;

    case ACTION_LEVEL:
      for (int i = 0; i < 4; i++) {
        int h = 4 + i * 3;
        tft.fillRect(cx - 8 + i * 5, cy + 6 - h, 3, h, color);
      }
      break;

    case ACTION_FILTER_POWER:
      tft.drawCircle(cx, cy, 8, color);
      tft.drawFastVLine(cx, cy - 8, 8, color);
      tft.drawFastHLine(cx - 6, cy + 5, 12, filterEnabled ? COL_GOLD : rgb565(62, 68, 82));
      break;

    case ACTION_FILTER_TYPE:
      tft.drawFastHLine(cx - 8, cy + 5, 16, rgb565(62, 68, 82));
      tft.drawLine(cx - 8, cy - 5, cx - 1, cy - 2, color);
      tft.drawLine(cx - 1, cy - 2, cx + 3, cy + 3, color);
      tft.drawLine(cx + 3, cy + 3, cx + 8, cy + 5, color);
      break;

    case ACTION_FILTER_CUTOFF:
      tft.drawFastHLine(cx - 8, cy + 5, 16, color);
      tft.drawFastVLine(cx, cy - 6, 12, COL_GOLD);
      tft.drawLine(cx - 8, cy - 2, cx, cy - 2, color);
      tft.drawLine(cx, cy - 2, cx + 8, cy + 4, color);
      break;

    case ACTION_ADSR:
      tft.drawLine(cx - 8, cy + 5, cx - 3, cy - 5, color);
      tft.drawLine(cx - 3, cy - 5, cx + 2, cy, color);
      tft.drawLine(cx + 2, cy, cx + 6, cy, color);
      tft.drawLine(cx + 6, cy, cx + 8, cy + 5, color);
      break;

    case ACTION_ATTACK:
      tft.drawLine(cx - 8, cy + 5, cx + 7, cy - 5, color);
      tft.drawFastHLine(cx - 8, cy + 5, 16, color);
      tft.fillTriangle(cx + 7, cy - 5, cx + 3, cy - 4, cx + 5, cy - 1, color);
      break;

    case ACTION_DECAY:
      tft.drawLine(cx - 8, cy - 5, cx + 7, cy + 5, color);
      tft.drawFastHLine(cx - 8, cy + 5, 16, color);
      tft.fillTriangle(cx + 7, cy + 5, cx + 3, cy + 4, cx + 5, cy + 1, color);
      break;

    case ACTION_SUSTAIN:
      tft.drawLine(cx - 8, cy - 4, cx - 2, cy, color);
      tft.drawFastHLine(cx - 2, cy, 10, color);
      tft.drawFastHLine(cx - 8, cy + 5, 16, rgb565(62, 68, 82));
      tft.fillCircle(cx + 7, cy, 2, color);
      break;

    case ACTION_RELEASE:
      tft.drawFastHLine(cx - 8, cy + 5, 16, rgb565(62, 68, 82));
      tft.drawLine(cx - 8, cy - 5, cx + 7, cy + 5, color);
      tft.fillTriangle(cx + 7, cy + 5, cx + 3, cy + 4, cx + 5, cy + 1, color);
      break;

    case ACTION_SPECTRUM:
      for (int i = 0; i < 5; i++) {
        int h = 3 + i * 2;
        tft.fillRect(cx - 8 + i * 4, cy + 6 - h, 2, h, color);
      }
      break;

    case ACTION_BT_FFT:
      tft.drawCircle(cx - 6, cy - 3, 2, color);
      tft.drawCircle(cx - 6, cy - 3, 5, rgb565(62, 68, 82));
      for (int i = 0; i < 4; i++) {
        int h = 3 + ((i + 1) % 4) * 3;
        tft.fillRect(cx - 1 + i * 4, cy + 6 - h, 2, h, color);
      }
      break;

    case ACTION_FFT_RANGE:
      tft.drawFastHLine(cx - 8, cy + 5, 16, color);
      tft.drawFastVLine(cx - 8, cy + 2, 4, color);
      tft.drawFastVLine(cx + 7, cy - 5, 11, color);
      tft.drawLine(cx - 8, cy + 4, cx + 7, cy - 5, color);
      break;

    case ACTION_FFT_WINDOW:
      for (int i = 0; i < 16; i++) {
        float p = (float)i / 15.0f;
        int h = (int)(sinf(p * 3.14159f) * 11.0f);
        tft.drawPixel(cx - 8 + i, cy + 6 - h, color);
      }
      break;

    case ACTION_FFT_SMOOTH:
      tft.drawLine(cx - 8, cy + 4, cx - 3, cy + 1, color);
      tft.drawLine(cx - 3, cy + 1, cx + 1, cy - 3, color);
      tft.drawLine(cx + 1, cy - 3, cx + 7, cy - 5, color);
      tft.drawFastHLine(cx - 7, cy + 6, 14, rgb565(62, 68, 82));
      break;

    case ACTION_FFT_HOLD:
      for (int i = 0; i < 4; i++) {
        int h = 4 + i * 3;
        int bx = cx - 7 + i * 5;
        tft.fillRect(bx, cy + 6 - h, 2, h, color);
        tft.drawFastHLine(bx - 1, cy + 3 - h, 4, COL_GOLD);
      }
      break;

    case ACTION_FFT_BINS:
      for (int i = 0; i < 4; i++) {
        tft.drawRect(cx - 8 + i * 4, cy - 5, 3, 11, color);
      }
      break;

    case ACTION_SCOPE:
      drawWavePreview(cx - 8, cy - 5, 16, 10, color);
      break;

    case ACTION_BT_RECEIVER:
      tft.drawCircle(cx, cy, 2, color);
      tft.drawCircle(cx, cy, 6, color);
      tft.drawCircle(cx, cy, 9, rgb565(62, 68, 82));
      tft.drawFastVLine(cx, cy + 2, 6, color);
      break;

    case ACTION_BT_DEVICE:
      tft.drawRoundRect(cx - 6, cy - 8, 12, 16, 2, color);
      tft.drawFastHLine(cx - 3, cy + 5, 6, color);
      tft.drawPixel(cx, cy + 7, color);
      break;

    case ACTION_BT_I2S:
      tft.drawFastHLine(cx - 8, cy - 4, 16, color);
      tft.drawFastHLine(cx - 8, cy, 16, color);
      tft.drawFastHLine(cx - 8, cy + 4, 16, color);
      tft.drawFastVLine(cx - 5, cy - 6, 12, rgb565(62, 68, 82));
      tft.drawFastVLine(cx + 5, cy - 6, 12, rgb565(62, 68, 82));
      break;

    case ACTION_BT_OUTPUT:
      tft.drawCircle(cx - 4, cy, 5, color);
      tft.drawCircle(cx + 5, cy, 5, color);
      tft.drawFastHLine(cx - 2, cy, 4, color);
      tft.fillCircle(cx - 4, cy, 2, color);
      tft.fillCircle(cx + 5, cy, 2, color);
      break;

    case ACTION_BT_POWER:
      tft.drawCircle(cx, cy, 8, color);
      tft.drawFastVLine(cx, cy - 8, 8, color);
      tft.drawPixel(cx - 1, cy - 1, COL_BG);
      tft.drawPixel(cx + 1, cy - 1, COL_BG);
      break;

    case ACTION_AUDIO_TEST:
      drawWavePreview(cx - 8, cy - 5, 16, 10, color);
      tft.drawFastVLine(cx + 9, cy - 7, 14, COL_GOLD);
      break;

    case ACTION_SEQ_START:
      tft.fillTriangle(cx - 5, cy - 7, cx - 5, cy + 7, cx + 7, cy, color);
      tft.drawCircle(cx, cy, 9, transportRunning ? COL_GOLD : rgb565(62, 68, 82));
      break;

    case ACTION_SEQ_STOP:
      tft.fillRect(cx - 6, cy - 6, 12, 12, color);
      tft.drawCircle(cx, cy, 9, !transportRunning ? COL_GOLD : rgb565(62, 68, 82));
      break;

    case ACTION_SEQ_POWER:
      for (int i = 0; i < 4; i++) {
        int bx = cx - 8 + i * 5;
        int h = 4 + i * 2;
        tft.fillRect(bx, cy + 6 - h, 3, h, color);
      }
      tft.drawFastHLine(cx - 8, cy + 7, 18, COL_GOLD);
      break;

    case ACTION_SEQ_BPM:
      tft.drawCircle(cx, cy, 8, color);
      tft.drawLine(cx, cy, cx + 5, cy - 5, color);
      tft.fillCircle(cx, cy, 2, COL_GOLD);
      break;

    case ACTION_SEQ_STEP:
      for (int i = 0; i < 4; i++) {
        tft.drawRect(cx - 8 + i * 5, cy - 5, 4, 10, color);
      }
      tft.fillRect(cx - 8, cy - 5, 4, 10, COL_GOLD);
      break;

    case ACTION_SEQ_NOTE:
      tft.drawFastVLine(cx - 5, cy - 7, 14, color);
      tft.drawFastHLine(cx - 5, cy - 7, 8, color);
      tft.drawFastHLine(cx - 5, cy - 1, 8, color);
      tft.fillCircle(cx + 5, cy + 5, 3, COL_GOLD);
      break;

    case ACTION_SEQ_LENGTH:
      for (int i = 0; i < 4; i++) {
        tft.drawRect(cx - 8 + i * 5, cy - 5, 4, 10, i < sequencerLength ? color : rgb565(62, 68, 82));
      }
      break;

    case ACTION_SEQ_DIRECTION:
      tft.drawLine(cx - 8, cy, cx + 6, cy, color);
      tft.fillTriangle(cx + 6, cy - 4, cx + 6, cy + 4, cx + 10, cy, color);
      if (sequencerDirection == 1) {
        tft.fillTriangle(cx - 8, cy - 4, cx - 8, cy + 4, cx - 12, cy, COL_GOLD);
      }
      break;

    case ACTION_SEQ_GATE:
      tft.drawRect(cx - 8, cy - 5, 16, 10, color);
      tft.fillRect(cx - 7, cy - 4, map(sequencerGate, 5, 100, 1, 14), 8, COL_GOLD);
      break;

    case ACTION_SEQ_TRANSPOSE:
      tft.drawFastVLine(cx, cy - 8, 16, color);
      tft.drawFastHLine(cx - 6, cy - 4, 13, color);
      tft.drawFastHLine(cx - 6, cy + 4, 13, color);
      tft.fillTriangle(cx + 8, cy - 3, cx + 8, cy + 3, cx + 12, cy, COL_GOLD);
      break;

    case ACTION_SEQ_STEP_ENABLE:
      tft.drawCircle(cx, cy, 8, color);
      if (sequencerStepEnabled[sequencerSelectedStep]) {
        tft.fillCircle(cx, cy, 5, COL_GOLD);
      } else {
        tft.drawLine(cx - 6, cy + 6, cx + 6, cy - 6, COL_CORAL);
      }
      break;

    case ACTION_SEQ_RANDOMIZE:
      for (int i = 0; i < 6; i++) {
        tft.drawPixel(cx - 7 + (i * 5) % 15, cy - 6 + (i * 7) % 13, color);
      }
      tft.drawCircle(cx, cy, 7, COL_GOLD);
      break;

    case ACTION_SEQ_CLEAR:
      tft.drawRect(cx - 8, cy - 6, 16, 12, color);
      tft.drawLine(cx - 6, cy - 4, cx + 6, cy + 4, COL_CORAL);
      tft.drawLine(cx + 6, cy - 4, cx - 6, cy + 4, COL_CORAL);
      break;

    case ACTION_SEQ_ROTATE:
      tft.drawCircle(cx, cy, 7, color);
      tft.fillTriangle(cx + 4, cy - 8, cx + 10, cy - 8, cx + 8, cy - 2, COL_GOLD);
      break;

    case ACTION_MIX_SINE:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[0], (float)waveVoiceFold[0], (float)waveVoicePhase[0], color);
      tft.drawFastHLine(cx - 8, cy + 7, map(waveVoiceLevels[0], 0, 100, 0, 16), COL_GOLD);
      break;

    case ACTION_MIX_SQUARE:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[1], (float)waveVoiceFold[1], (float)waveVoicePhase[1], color);
      tft.drawFastHLine(cx - 8, cy + 7, map(waveVoiceLevels[1], 0, 100, 0, 16), COL_GOLD);
      break;

    case ACTION_MIX_TRIANGLE:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[2], (float)waveVoiceFold[2], (float)waveVoicePhase[2], color);
      tft.drawFastHLine(cx - 8, cy + 7, map(waveVoiceLevels[2], 0, 100, 0, 16), COL_GOLD);
      break;

    case ACTION_MIX_SAW:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[3], (float)waveVoiceFold[3], (float)waveVoicePhase[3], color);
      tft.drawFastHLine(cx - 8, cy + 7, map(waveVoiceLevels[3], 0, 100, 0, 16), COL_GOLD);
      break;

    case ACTION_MIX_KICK:
      tft.drawCircle(cx, cy, 8, color);
      tft.fillCircle(cx, cy, 4, COL_GOLD);
      tft.drawFastHLine(cx - 10, cy + 8, 20, color);
      break;

    case ACTION_KICK_PITCH:
      tft.drawCircle(cx, cy, 7, color);
      tft.drawLine(cx, cy, cx + 4, cy - 5, COL_GOLD);
      tft.drawFastHLine(cx - 8, cy + 8, 16, color);
      break;

    case ACTION_KICK_DECAY:
      tft.drawLine(cx - 8, cy - 5, cx + 7, cy + 5, color);
      tft.drawFastHLine(cx - 8, cy + 5, 16, COL_GOLD);
      break;

    case ACTION_KICK_SWEEP:
      tft.drawLine(cx - 8, cy + 5, cx - 2, cy - 3, color);
      tft.drawLine(cx - 2, cy - 3, cx + 8, cy - 5, COL_GOLD);
      break;

    case ACTION_KICK_CLICK:
      tft.drawFastVLine(cx - 4, cy - 7, 14, COL_GOLD);
      tft.drawLine(cx - 4, cy - 7, cx + 8, cy + 5, color);
      break;

    case ACTION_KICK_DRIVE:
      tft.drawRect(cx - 8, cy - 6, 16, 12, color);
      tft.drawLine(cx - 7, cy + 4, cx - 2, cy - 4, COL_GOLD);
      tft.drawLine(cx - 2, cy - 4, cx + 3, cy + 4, COL_GOLD);
      tft.drawLine(cx + 3, cy + 4, cx + 7, cy - 3, COL_GOLD);
      break;

    case ACTION_INPUTS:
      tft.drawCircle(cx - 4, cy - 2, 3, color);
      tft.drawCircle(cx + 5, cy + 3, 3, color);
      tft.drawLine(cx - 1, cy - 1, cx + 2, cy + 2, color);
      break;

    case ACTION_DISPLAY_TEST:
      tft.drawRect(cx - 7, cy - 5, 14, 10, color);
      tft.drawFastHLine(cx - 4, cy + 7, 8, color);
      break;

    case ACTION_ABOUT:
      tft.drawCircle(cx, cy, 6, color);
      tft.setTextColor(color);
      tft.setTextSize(1);
      tft.setCursor(cx - 2, cy - 4);
      tft.print("i");
      break;
  }
}

void drawGrooveBar(int x, int y, int width, int height, int fillWidth, uint16_t color) {
  int innerW = width - 2;
  int clampedFill = clampInt(fillWidth, 0, innerW);

  tft.fillRect(x, y, width, height, COL_PANEL);
  tft.drawRect(x, y, width, height, COL_LINE);

  for (int i = 1; i < 8; i++) {
    int tickX = x + 1 + (innerW * i) / 8;
    tft.drawFastVLine(tickX, y + 1, height - 2, COL_LINE);
  }

  if (clampedFill > 0) {
    tft.fillRect(x + 1, y + 1, clampedFill, height - 2, color);
  }

  int needleX = x + 1 + clampedFill;
  if (needleX >= x + width) needleX = x + width - 2;

  tft.drawFastVLine(needleX, y - 2, height + 4, COL_INK);
}

void drawPanelFrame(int x, int y, int width, int height, uint16_t accent) {
  tft.fillRect(x, y, width, height, COL_PANEL);
  tft.drawRect(x, y, width, height, COL_LINE);
  tft.drawFastHLine(x, y, width, accent);
}

void drawValueReadout(const char* value, const char* tag, uint16_t accent) {
  tft.setTextSize(2);
  tft.setTextColor(accent);
  tft.setCursor(10, 56);
  tft.print(value);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(108, 59);
  tft.print(tag);
}

void drawScopeGrid(int x, int y, int width, int height) {
  for (int i = 1; i < 4; i++) {
    int gx = x + (width * i) / 4;
    tft.drawFastVLine(gx, y + 1, height - 2, rgb565(24, 29, 43));
  }

  for (int i = 1; i < 3; i++) {
    int gy = y + (height * i) / 3;
    tft.drawFastHLine(x + 1, gy, width - 2, rgb565(24, 29, 43));
  }
}

void drawSpectrumBars(int x, int y, int width, int height, uint16_t accent, bool normalizeOutput) {
  float magnitudes[SPECTRUM_MAX_BARS];
  int bins = getSpectrumBarCount();
  int maxH = height - 4;
  int markerIndex = clampInt((int)getSpectrumPosition(frequency), 0, bins - 1);
  int markerX = x + (markerIndex * width) / bins + (width / bins) / 2;

  buildOscillatorSpectrum(magnitudes, normalizeOutput);

  for (int i = 0; i < bins; i++) {
    int barH = clampInt((int)(magnitudes[i] * maxH), 1, maxH);
    int barLeft = x + (i * width) / bins;
    int barRight = x + ((i + 1) * width) / bins;
    int bx = barLeft + 1;
    int by = y + height - 2 - barH;
    int fillW = barRight - barLeft - 2;
    if (fillW < 1) fillW = 1;

    uint16_t color = i == markerIndex ? accent : rgb565(54, 63, 79);

    if (magnitudes[i] > 0.55f) {
      color = i == markerIndex ? COL_GOLD : COL_SKY;
    }

    tft.fillRect(bx, by, fillW, barH, color);

    if (fftPeakHold && magnitudes[i] > 0.08f) {
      int capY = clampInt(by - 2 - (i % 3), y + 2, y + height - 3);
      tft.drawFastHLine(bx, capY, fillW, COL_GOLD);
    }
  }

  markerX = clampInt(markerX, x + 2, x + width - 3);
  tft.drawFastVLine(markerX, y + 2, height - 4, COL_GOLD);
}

bool buildBluetoothFftMagnitudes(uint8_t magnitudes[], int count) {
  int16_t samples[BT_FFT_SAMPLES];
  uint32_t available = btFftSampleCount;
  uint16_t writeIndex = btFftWriteIndex;

  if (available > BT_FFT_SAMPLES) {
    available = BT_FFT_SAMPLES;
  }

  for (int i = 0; i < BT_FFT_SAMPLES; i++) {
    samples[i] = 0;
  }

  if (available < 16) {
    for (int i = 0; i < count; i++) {
      magnitudes[i] = 0;
    }

    return false;
  }

  uint16_t start = (writeIndex + BT_FFT_SAMPLES - available) % BT_FFT_SAMPLES;
  int pad = BT_FFT_SAMPLES - available;
  int32_t mean = 0;

  for (uint32_t i = 0; i < available; i++) {
    int16_t sample = btFftSamples[(start + i) % BT_FFT_SAMPLES];
    samples[pad + i] = sample;
    mean += sample;
  }

  mean /= (int32_t)available;

  for (int bin = 0; bin < count; bin++) {
    int maxK = BT_FFT_SAMPLES / 2 - 2;
    int k = 1;

    if (count > 1) {
      long curved = (long)bin * (long)bin * maxK;
      k += curved / ((long)(count - 1) * (long)(count - 1));
    }

    float real = 0.0f;
    float imag = 0.0f;

    for (int n = 0; n < BT_FFT_SAMPLES; n++) {
      float window = 0.5f - 0.5f * cosf(TWO_PI_F * n / (BT_FFT_SAMPLES - 1));
      float sample = ((float)samples[n] - mean) / 32768.0f * window;
      float angle = TWO_PI_F * k * n / BT_FFT_SAMPLES;

      real += sample * cosf(angle);
      imag -= sample * sinf(angle);
    }

    float magnitude = sqrtf(real * real + imag * imag) / (BT_FFT_SAMPLES * 0.18f);
    magnitudes[bin] = (uint8_t)clampInt((int)(magnitude * 100.0f), 0, 100);
  }

  return true;
}

void drawBluetoothFftBars(int x, int y, int width, int height, uint16_t accent) {
  uint8_t magnitudes[BT_FFT_BARS];
  bool hasSamples = buildBluetoothFftMagnitudes(magnitudes, BT_FFT_BARS);
  int maxH = height - 4;

  for (int i = 0; i < BT_FFT_BARS; i++) {
    int barLeft = x + (i * width) / BT_FFT_BARS;
    int barRight = x + ((i + 1) * width) / BT_FFT_BARS;
    int fillW = barRight - barLeft - 2;
    if (fillW < 1) fillW = 1;

    int level = hasSamples ? magnitudes[i] : 4 + (i % 4) * 2;
    int barH = clampInt(map(level, 0, 100, 1, maxH), 1, maxH);
    int bx = barLeft + 1;
    int by = y + height - 2 - barH;
    uint16_t color = hasSamples ? (level > 72 ? COL_GOLD : accent) : rgb565(54, 63, 79);

    tft.fillRect(bx, by, fillW, barH, color);
  }
}

void drawOscilloscopeTrace(int x, int y, int width, int height, uint16_t accent, bool useLevel) {
  float amplitude = useLevel ? volume / 100.0f : 1.0f;
  int centerY = y + height / 2;
  int prevY = centerY;

  for (int i = 0; i < width; i++) {
    float phase = (float)i / (float)(width - 1) * 2.0f;
    float sample = getOscillatorSample(phase) * amplitude;
    int nextY = centerY - (int)(sample * (height / 2 - 3));

    if (i > 0) {
      tft.drawLine(x + i - 1, prevY, x + i, nextY, accent);
    }

    prevY = nextY;
  }
}

const char* getWaveformShortName(int shape) {
  switch (shape) {
    case 0: return "SIN";
    case 1: return "SQR";
    case 2: return "TRI";
    case 3: return "SAW";
    case 4: return "RMP";
    case 5: return "PLS";
    case 6: return "SUB";
    case 7: return "ORG";
    case 8: return "EVN";
    case 9: return "ODD";
    case 10: return "SUP";
    case 11: return "PWM";
    case 12: return "STP";
    case 13: return "CLP";
    case 14: return "NOI";
    case 15: return "FRM";
    default: return "---";
  }
}

void drawWaveShapeTabs(int selectedShape, int y, uint16_t accent) {
  int firstShape = selectedShape - 1;
  int maxFirst = WAVE_SHAPE_COUNT - 4;
  if (maxFirst < 0) maxFirst = 0;
  firstShape = clampInt(firstShape, 0, maxFirst);

  for (int i = 0; i < 4; i++) {
    int shape = firstShape + i;
    if (shape >= WAVE_SHAPE_COUNT) break;

    int x = 10 + i * 37;
    bool active = shape == selectedShape;

    if (active) {
      tft.fillRect(x - 2, y - 2, 32, 12, accent);
      tft.setTextColor(COL_BG);
    } else {
      tft.drawRect(x - 2, y - 2, 32, 12, COL_LINE);
      tft.setTextColor(COL_MUTED);
    }

    tft.setTextSize(1);
    tft.setCursor(x + 1, y + 1);
    tft.print(getWaveformShortName(shape));
  }
}

void drawEnvelopeCurve(bool attackMode, uint16_t accent) {
  int x = 12;
  int y = 79;
  int width = 136;
  int height = 30;
  int baseY = y + height - 3;
  int peakY = y + 3;
  int totalMs = attack + decay;
  if (totalMs < 1) totalMs = 1;
  int attackX = x + map(attack, 0, totalMs, 2, width - 4);

  drawPanelFrame(10, 76, 140, 38, accent);

  tft.drawFastHLine(x, baseY, width, rgb565(34, 40, 53));
  tft.drawFastVLine(x, peakY, height, rgb565(34, 40, 53));
  tft.drawLine(x + 2, baseY, attackX, peakY, attackMode ? accent : COL_SKY);
  tft.drawLine(attackX, peakY, x + width - 4, baseY, attackMode ? COL_SKY : accent);

  if (attackMode) {
    tft.fillCircle(attackX, peakY, 2, COL_GOLD);
  } else {
    tft.fillCircle(x + width - 4, baseY, 2, COL_GOLD);
  }

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 118);
  tft.print("A");
  tft.print(attack);
  tft.print(" D");
  tft.print(decay);
}

void drawAdsrCurve(uint16_t accent) {
  int x = 12;
  int y = 76;
  int width = 136;
  int height = 42;
  int baseY = y + height - 5;
  int peakY = y + 5;
  int sustainY = baseY - map(sustain, 0, 100, 0, height - 10);

  int attackX = x + map(attack, 0, 500, 14, 42);
  int decayX = attackX + map(decay, 0, 2000, 18, 42);
  int sustainX = decayX + 30;
  int releaseX = x + width - 6;

  if (decayX > x + 92) decayX = x + 92;
  if (sustainX > x + 116) sustainX = x + 116;

  drawPanelFrame(10, 73, 140, 47, accent);

  tft.drawFastHLine(x, baseY, width, rgb565(34, 40, 53));
  tft.drawFastVLine(x, peakY, height - 8, rgb565(34, 40, 53));

  MenuAction focus = getSelectedAction();
  bool overview = focus == ACTION_ADSR;
  uint16_t dim = rgb565(56, 62, 76);
  uint16_t attackColor = overview || focus == ACTION_ATTACK ? accent : dim;
  uint16_t decayColor = overview ? COL_SKY : focus == ACTION_DECAY ? accent : dim;
  uint16_t sustainColor = overview ? COL_GOLD : focus == ACTION_SUSTAIN ? accent : dim;
  uint16_t releaseColor = overview ? COL_CORAL : focus == ACTION_RELEASE ? accent : dim;

  tft.drawLine(x + 2, baseY, attackX, peakY, attackColor);
  tft.drawLine(attackX, peakY, decayX, sustainY, decayColor);
  tft.drawLine(decayX, sustainY, sustainX, sustainY, sustainColor);
  tft.drawLine(sustainX, sustainY, releaseX, baseY, releaseColor);

  tft.fillCircle(attackX, peakY, 2, attackColor);
  tft.fillCircle(decayX, sustainY, 2, decayColor);
  tft.fillCircle(sustainX, sustainY, 2, sustainColor);

  tft.setTextSize(1);
  char label[12];

  tft.setTextColor(overview || focus == ACTION_ATTACK ? attackColor : COL_MUTED);
  snprintf(label, sizeof(label), "A%d", attack);
  tft.setCursor(12, 122);
  tft.print(label);

  tft.setTextColor(overview || focus == ACTION_DECAY ? decayColor : COL_MUTED);
  snprintf(label, sizeof(label), "D%d", decay);
  tft.setCursor(44, 122);
  tft.print(label);

  tft.setTextColor(overview || focus == ACTION_SUSTAIN ? sustainColor : COL_MUTED);
  snprintf(label, sizeof(label), "S%d", sustain);
  tft.setCursor(84, 122);
  tft.print(label);

  snprintf(label, sizeof(label), "R%d", releaseMs);
  printRight(label, 150, 122, 1, overview || focus == ACTION_RELEASE ? releaseColor : COL_MUTED);
}

void drawSpectrumScale(int y) {
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, y);
  tft.print("20");
  printRight(getSpectrumMaxLabel(), 149, y, 1, COL_MUTED);
}

void drawWindowCurve(int x, int y, int width, int height, uint16_t accent) {
  int prevX = x;
  int prevY = y + height - 3;

  for (int i = 0; i < width; i++) {
    float p = (float)i / (float)(width - 1);
    float value = 1.0f;

    if (fftWindow == 0) {
      value = 0.5f - 0.5f * cosf(TWO_PI_F * p);
    } else if (fftWindow == 2) {
      value = 0.42f - 0.5f * cosf(TWO_PI_F * p) + 0.08f * cosf(TWO_PI_F * 2.0f * p);
    } else if (fftWindow == 3) {
      value = 0.22f - 0.42f * cosf(TWO_PI_F * p) + 0.28f * cosf(TWO_PI_F * 2.0f * p) - 0.08f * cosf(TWO_PI_F * 3.0f * p);
      value = clampInt((int)(value * 100.0f), 0, 100) / 100.0f;
    }

    int nextX = x + i;
    int nextY = y + height - 3 - (int)(value * (height - 6));

    if (i > 0) {
      tft.drawLine(prevX, prevY, nextX, nextY, accent);
    }

    prevX = nextX;
    prevY = nextY;
  }
}

void drawPitchEdit(uint16_t accent) {
  char pitchLabel[12];
  getPitchLabel(pitchLabel, sizeof(pitchLabel));

  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "PITCH", accent);
  printRight(pitchLabel, 150, 67, 1, COL_INK);
  drawPanelFrame(10, 74, 140, 41, accent);
  drawSpectrumBars(14, 78, 132, 33, accent, true);

  drawSpectrumScale(120);
}

void drawLevelEdit(uint16_t accent) {
  char dbText[12];
  snprintf(dbText, sizeof(dbText), "%ddB", getRmsDbFs());

  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "RMS", accent);
  printRight(dbText, 150, 67, 1, COL_INK);

  int leftFill = map(volume, 0, 100, 0, 62);
  int rightFill = map(clampInt((int)(getOscillatorRms() * 100.0f), 0, 100), 0, 100, 0, 62);

  drawGrooveBar(10, 78, 66, 9, leftFill, accent);
  drawGrooveBar(84, 78, 66, 9, rightFill, COL_SKY);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 94);
  tft.print("PEAK");
  tft.setCursor(84, 94);
  tft.print("RMS");

  drawPanelFrame(10, 103, 140, 22, accent);
  drawSpectrumBars(14, 107, 132, 14, accent, false);
}

uint16_t getMixerVoiceColor(int voiceIndex, uint16_t accent);

void drawVoiceOscilloscopeTrace(int x, int y, int width, int height, int voice, uint16_t accent) {
  int centerY = y + height / 2;
  int prevY = centerY;

  for (int i = 0; i < width; i++) {
    float phase = (float)i / (float)(width - 1) * 2.0f;
    float sample = getOscillatorSampleForParams(phase, waveVoiceShape[voice], (float)waveVoiceFold[voice], (float)waveVoicePhase[voice]);
    int nextY = centerY - (int)(sample * (height / 2 - 3));

    if (i > 0) {
      tft.drawLine(x + i - 1, prevY, x + i, nextY, accent);
    }

    prevY = nextY;
  }
}

void drawVoiceScopePanel(int voice, uint16_t accent) {
  drawPanelFrame(10, 74, 140, 34, accent);
  drawScopeGrid(10, 74, 140, 34);
  drawVoiceOscilloscopeTrace(14, 78, 132, 26, voice, accent);
}

void drawVoiceWaveEdit(uint16_t accent) {
  int voice = getCurrentWaveVoiceIndex();
  int shape = waveVoiceShape[voice];

  drawValueReadout(waveformNames[shape], "WAVE", accent);
  char voiceText[6];
  snprintf(voiceText, sizeof(voiceText), "V%d", voice + 1);
  printRight(voiceText, 150, 67, 1, getMixerVoiceColor(voice, accent));
  drawVoiceScopePanel(voice, accent);
  drawWaveShapeTabs(shape, 114, accent);
}

void drawVoiceTuneEdit(uint16_t accent) {
  int voice = getCurrentWaveVoiceIndex();
  char value[12];
  snprintf(value, sizeof(value), "%+dst", waveVoiceTune[voice]);

  drawValueReadout(value, "TUNE", accent);
  printRight(waveformNames[waveVoiceShape[voice]], 150, 67, 1, getMixerVoiceColor(voice, accent));
  drawVoiceScopePanel(voice, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 112);
  tft.print("-24");
  printRight("+24", 150, 112, 1, COL_MUTED);
  drawGrooveBar(50, 115, 60, 8, map(waveVoiceTune[voice], -24, 24, 0, 58), accent);
}

void drawVoiceFoldEdit(uint16_t accent) {
  int voice = getCurrentWaveVoiceIndex();
  char value[12];
  int driveTenths = 10 + (waveVoiceFold[voice] * 24) / 100;
  snprintf(value, sizeof(value), "%d%%", waveVoiceFold[voice]);

  drawValueReadout(value, "FOLD", accent);
  char driveText[10];
  snprintf(driveText, sizeof(driveText), "x%d.%d", driveTenths / 10, driveTenths % 10);
  printRight(driveText, 150, 67, 1, COL_INK);
  drawVoiceScopePanel(voice, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 112);
  tft.print("CLEAN");
  printRight("FOLD", 150, 112, 1, COL_MUTED);
  drawGrooveBar(50, 115, 60, 8, map(waveVoiceFold[voice], 0, 100, 0, 58), accent);
}

void drawVoicePhaseEdit(uint16_t accent) {
  int voice = getCurrentWaveVoiceIndex();
  char value[12];
  snprintf(value, sizeof(value), "%ddeg", waveVoicePhase[voice]);

  drawValueReadout(value, "PHASE", accent);

  drawPanelFrame(10, 74, 56, 48, accent);
  int cx = 38;
  int cy = 98;
  float angle = (waveVoicePhase[voice] - 90) * TWO_PI_F / 360.0f;
  int mx = cx + (int)(cosf(angle) * 17.0f);
  int my = cy + (int)(sinf(angle) * 17.0f);

  tft.drawCircle(cx, cy, 18, COL_LINE);
  tft.drawCircle(cx, cy, 12, rgb565(34, 40, 53));
  tft.drawLine(cx, cy, mx, my, accent);
  tft.fillCircle(mx, my, 2, COL_GOLD);

  drawPanelFrame(74, 74, 76, 48, accent);
  drawScopeGrid(74, 74, 76, 48);
  drawVoiceOscilloscopeTrace(78, 80, 68, 35, voice, accent);
}

void drawVoiceScopeEdit(uint16_t accent) {
  int voice = getCurrentWaveVoiceIndex();
  drawValueReadout("LIVE", "SCOPE", accent);
  printRight(waveformNames[waveVoiceShape[voice]], 150, 67, 1, getMixerVoiceColor(voice, accent));
  drawVoiceScopePanel(voice, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("T");
  tft.print(waveVoiceTune[voice]);
  tft.print(" F");
  tft.print(waveVoiceFold[voice]);
  printRight(waveVoiceLevels[voice] > 0 ? "in mix" : "muted", 149, 120, 1, waveVoiceLevels[voice] > 0 ? COL_GOLD : COL_MUTED);
}

int getFilterCutoffMarkerX(int x, int width) {
  float minHz = (float)FILTER_MIN_CUTOFF_HZ;
  float maxHz = (float)FILTER_MAX_CUTOFF_HZ;
  float cutoff = clampFloat((float)filterCutoff, minHz, maxHz);
  float position = logf(cutoff / minHz) / logf(maxHz / minHz);

  return x + clampInt((int)(position * (width - 1)), 0, width - 1);
}

void drawFilterCurve(uint16_t accent) {
  int x = 10;
  int y = 74;
  int width = 140;
  int height = 42;
  int plotX = x + 8;
  int plotY = y + 6;
  int plotW = width - 16;
  int plotH = height - 13;
  int lowY = plotY + plotH - 2;
  int highY = plotY + 2;
  int midY = plotY + plotH / 2;
  int cutX = getFilterCutoffMarkerX(plotX, plotW);
  uint16_t curveColor = filterEnabled ? accent : rgb565(62, 68, 82);

  drawPanelFrame(x, y, width, height, accent);
  drawScopeGrid(x, y, width, height);
  tft.drawFastHLine(plotX, lowY, plotW, rgb565(34, 40, 53));
  tft.drawFastVLine(plotX, highY, plotH, rgb565(34, 40, 53));

  if (filterType == 1) {
    int kneeX = min(plotX + plotW - 1, cutX + 18);
    tft.drawLine(plotX, lowY, cutX, lowY, curveColor);
    tft.drawLine(cutX, lowY, kneeX, highY, curveColor);
    tft.drawLine(kneeX, highY, plotX + plotW - 1, highY, curveColor);
  } else if (filterType == 2) {
    int leftX = max(plotX, cutX - 28);
    int rightX = min(plotX + plotW - 1, cutX + 28);
    tft.drawLine(plotX, lowY, leftX, lowY, curveColor);
    tft.drawLine(leftX, lowY, cutX, highY, curveColor);
    tft.drawLine(cutX, highY, rightX, lowY, curveColor);
    tft.drawLine(rightX, lowY, plotX + plotW - 1, lowY, curveColor);
  } else {
    int kneeX = min(plotX + plotW - 1, cutX + 18);
    tft.drawLine(plotX, highY, cutX, highY, curveColor);
    tft.drawLine(cutX, highY, kneeX, lowY, curveColor);
    tft.drawLine(kneeX, lowY, plotX + plotW - 1, lowY, curveColor);
  }

  tft.drawFastVLine(cutX, plotY, plotH, COL_GOLD);
  tft.fillCircle(cutX, filterType == 2 ? highY : midY, 2, COL_GOLD);
}

void drawFilterPowerEdit(uint16_t accent) {
  drawValueReadout(filterEnabled ? "ON" : "OFF", "FILTER", accent);
  printRight(FILTER_TYPE_NAMES[filterType], 150, 67, 1, filterEnabled ? COL_GOLD : COL_MUTED);
  drawFilterCurve(accent);
  printRight(filterEnabled ? "active" : "bypass", 149, 120, 1, filterEnabled ? COL_GOLD : COL_MUTED);
}

void drawFilterTypeEdit(uint16_t accent) {
  drawValueReadout(FILTER_TYPE_NAMES[filterType], "TYPE", accent);
  printRight(filterEnabled ? "active" : "bypass", 150, 67, 1, filterEnabled ? COL_GOLD : COL_MUTED);
  drawFilterCurve(accent);
  printRight("LP HP BP", 149, 120, 1, COL_MUTED);
}

void drawFilterCutoffEdit(uint16_t accent) {
  char cutoffText[14];
  snprintf(cutoffText, sizeof(cutoffText), "%dHz", filterCutoff);

  drawValueReadout(cutoffText, "CUTOFF", accent);
  printRight(FILTER_TYPE_NAMES[filterType], 150, 67, 1, filterEnabled ? COL_GOLD : COL_INK);
  drawFilterCurve(accent);

  int fill = map(filterCutoff, FILTER_MIN_CUTOFF_HZ, FILTER_MAX_CUTOFF_HZ, 0, 58);
  drawGrooveBar(50, 116, 60, 8, fill, accent);
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 116);
  tft.print("40");
  printRight("12K", 150, 116, 1, COL_MUTED);
}

void drawAdsrEdit(uint16_t accent) {
  drawValueReadout("VIEW", "ADSR", accent);
  printRight("overview", 150, 67, 1, COL_INK);
  drawAdsrCurve(accent);
}

void drawAttackEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "ENV A", accent);
  printRight("attack", 150, 67, 1, COL_INK);
  drawAdsrCurve(accent);
}

void drawDecayEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "ENV D", accent);
  printRight("decay", 150, 67, 1, COL_INK);
  drawAdsrCurve(accent);
}

void drawSustainEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "ENV S", accent);
  printRight("sustain", 150, 67, 1, COL_INK);
  drawAdsrCurve(accent);
}

void drawReleaseEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "ENV R", accent);
  printRight("release", 150, 67, 1, COL_INK);
  drawAdsrCurve(accent);
}

void drawSpectrumEdit(uint16_t accent) {
  char pitchLabel[12];
  getPitchLabel(pitchLabel, sizeof(pitchLabel));

  drawValueReadout("HARM", "SPEC", accent);
  printRight(pitchLabel, 150, 67, 1, COL_INK);
  drawPanelFrame(10, 74, 140, 41, accent);
  drawSpectrumBars(14, 78, 132, 33, accent, true);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print(waveformNames[waveform]);
  tft.print(" ");
  tft.print(FFT_BIN_VALUES[fftBinsIndex]);
  tft.print("b");
  printRight(getSpectrumMaxLabel(), 149, 120, 1, COL_MUTED);
}

void drawBluetoothFftEdit(uint16_t accent) {
  bool streaming = isBluetoothStreaming();
  bool connected = bluetoothEnabled && btAudioStarted && a2dpSink.is_connected();
  const char* status = getBluetoothStatusText();

  drawValueReadout(status, "BT FFT", accent);
  printRight(streaming ? "pcm stream" : connected ? "waiting audio" : "not linked", 150, 67, 1, streaming ? COL_GOLD : COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawBluetoothFftBars(14, 78, 132, 33, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("BT");
  tft.setTextColor(streaming ? COL_GOLD : COL_MUTED);
  tft.print(streaming ? " live" : connected ? " linked" : " pair");
  printRight("PCM FFT", 149, 120, 1, COL_MUTED);
}

void drawFftRangeEdit(uint16_t accent) {
  char value[14];
  snprintf(value, sizeof(value), "20-%s", getSpectrumMaxLabel());

  drawValueReadout(value, "RANGE", accent);
  printRight("log bins", 150, 67, 1, COL_INK);
  drawPanelFrame(10, 74, 140, 41, accent);
  drawSpectrumBars(14, 78, 132, 33, accent, true);
  drawSpectrumScale(120);
}

void drawFftWindowEdit(uint16_t accent) {
  drawValueReadout(FFT_WINDOW_NAMES[fftWindow], "WINDOW", accent);
  printRight("leakage", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 65, 43, accent);
  drawWindowCurve(14, 80, 57, 31, accent);

  drawPanelFrame(84, 74, 66, 43, accent);
  drawSpectrumBars(88, 79, 58, 34, accent, true);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(12, 121);
  tft.print("window");
  printRight(FFT_WINDOW_NAMES[fftWindow], 149, 121, 1, COL_MUTED);
}

void drawFftSmoothEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "AVG", accent);
  printRight("temporal", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 34, accent);
  drawSpectrumBars(14, 78, 132, 26, accent, true);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 112);
  tft.print("FAST");
  printRight("STABLE", 150, 112, 1, COL_MUTED);
  drawGrooveBar(50, 115, 60, 8, map(fftSmoothing, 0, 95, 0, 58), accent);
}

void drawFftHoldEdit(uint16_t accent) {
  drawValueReadout(fftPeakHold ? "ON" : "OFF", "HOLD", accent);
  printRight("peak caps", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 41, accent);
  drawSpectrumBars(14, 78, 132, 33, accent, true);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("hold ");
  tft.setTextColor(fftPeakHold ? COL_GOLD : COL_MUTED);
  tft.print(fftPeakHold ? "armed" : "bypassed");
}

void drawFftBinsEdit(uint16_t accent) {
  char value[10];
  snprintf(value, sizeof(value), "%d BIN", FFT_BIN_VALUES[fftBinsIndex]);

  drawValueReadout(value, "RES", accent);
  printRight("display", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 41, accent);
  drawSpectrumBars(14, 78, 132, 33, accent, true);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("range ");
  tft.print(getSpectrumMaxLabel());
  printRight("log FFT", 149, 120, 1, COL_MUTED);
}

void drawScopeEdit(uint16_t accent) {
  drawValueReadout("LIVE", "SCOPE", accent);
  drawPanelFrame(10, 74, 140, 42, accent);
  drawScopeGrid(10, 74, 140, 42);
  drawOscilloscopeTrace(14, 79, 132, 33, accent, true);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print(waveformNames[waveform]);
  tft.print(" ");
  tft.print(volume);
  tft.print("%");
}

void drawAudioTestEdit(uint16_t accent) {
  drawValueReadout(waveformNames[waveform], "TEST", accent);
  printRight("DAC wave", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawScopeGrid(10, 74, 140, 42);
  drawOscilloscopeTrace(14, 79, 132, 33, accent, false);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print(frequency);
  tft.print("Hz ");
  tft.print(volume);
  tft.print("%");
  printRight(isSynthAudioActive() ? "I2S ON" : "I2S OFF", 149, 120, 1, isSynthAudioActive() ? COL_GOLD : COL_MUTED);
}

void drawSequencerGrid(int x, int y, int width, int height, uint16_t accent) {
  int cellW = width / SEQUENCER_STEP_COUNT;
  int range = SEQUENCER_MAX_NOTE - SEQUENCER_MIN_NOTE;
  int length = getSequencerLength();

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    int left = x + i * cellW;
    int right = (i == SEQUENCER_STEP_COUNT - 1) ? x + width : left + cellW;
    int w = right - left - 2;
    if (w < 2) w = 2;

    bool selected = i == sequencerSelectedStep;
    bool playing = transportRunning && sequencerEnabled && i == sequencerCurrentStep;
    bool inLength = i < length;
    bool enabled = sequencerStepEnabled[i];
    uint16_t frameColor = !inLength ? rgb565(30, 34, 45) : playing ? COL_GOLD : selected ? accent : COL_LINE;
    uint16_t fillColor = !enabled ? rgb565(42, 47, 62) : playing ? COL_GOLD : selected ? accent : rgb565(54, 63, 79);

    tft.drawRect(left, y, w, height, frameColor);
    if (!inLength) continue;

    int note = clampInt(sequencerStepNotes[i], SEQUENCER_MIN_NOTE, SEQUENCER_MAX_NOTE);
    int noteY = y + height - 4 - ((note - SEQUENCER_MIN_NOTE) * (height - 8)) / range;
    int barH = y + height - 3 - noteY;
    if (barH < 2) barH = 2;

    if (enabled) {
      tft.fillRect(left + 2, y + height - 2 - barH, w - 4, barH, fillColor);
    } else {
      tft.drawFastHLine(left + 2, y + height / 2, w - 4, COL_CORAL);
    }

    if (playing) {
      tft.drawFastHLine(left + 2, y + 2, w - 4, COL_INK);
    }
  }
}

void formatSequencerNote(char* buffer, size_t size, int note) {
  snprintf(buffer, size, "%+dst", note);
}

void drawSequencerStartEdit(uint16_t accent) {
  drawValueReadout("START", "SEQ", accent);
  printRight(transportRunning ? "running" : "armed", 150, 67, 1, transportRunning ? COL_GOLD : COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawSequencerGrid(14, 79, 132, 32, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("step ");
  tft.print(sequencerCurrentStep + 1);
  printRight(sequencerEnabled ? "SEQ ON" : "SEQ OFF", 149, 120, 1, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerStopEdit(uint16_t accent) {
  drawValueReadout("STOP", "SEQ", accent);
  printRight(transportRunning ? "running" : "stopped", 150, 67, 1, transportRunning ? COL_INK : COL_GOLD);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawSequencerGrid(14, 79, 132, 32, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("transport");
  printRight(transportRunning ? "PLAY" : "SILENT", 149, 120, 1, transportRunning ? COL_MUTED : COL_GOLD);
}

void drawSequencerRunEdit(uint16_t accent) {
  drawValueReadout(sequencerEnabled ? "ON" : "OFF", "SEQ", accent);
  printRight(transportRunning && sequencerEnabled ? "running" : "stopped", 150, 67, 1, transportRunning && sequencerEnabled ? COL_GOLD : COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawSequencerGrid(14, 79, 132, 32, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("step ");
  tft.print(sequencerCurrentStep + 1);
  printRight(waveformNames[waveform], 149, 120, 1, COL_MUTED);
}

void drawSequencerBpmEdit(uint16_t accent) {
  char value[12];
  snprintf(value, sizeof(value), "%d", sequencerBpm);

  drawValueReadout(value, "BPM", accent);
  printRight("16th clock", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawSequencerGrid(14, 79, 132, 32, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("rate");
  printRight(sequencerEnabled ? "SEQ ON" : "SEQ OFF", 149, 120, 1, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerStepEdit(uint16_t accent) {
  char value[8];
  snprintf(value, sizeof(value), "S%d", sequencerSelectedStep + 1);

  drawValueReadout(value, "STEP", accent);
  printRight("select", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawSequencerGrid(14, 79, 132, 32, accent);

  char noteText[10];
  formatSequencerNote(noteText, sizeof(noteText), sequencerStepNotes[sequencerSelectedStep]);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("note ");
  tft.print(noteText);
  printRight("edit target", 149, 120, 1, COL_MUTED);
}

void drawSequencerNoteEdit(uint16_t accent) {
  char value[10];
  formatSequencerNote(value, sizeof(value), sequencerStepNotes[sequencerSelectedStep]);

  drawValueReadout(value, "NOTE", accent);
  printRight("semitone", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawSequencerGrid(14, 79, 132, 32, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("base ");
  tft.print(frequency);
  tft.print("Hz");
  printRight(sequencerEnabled ? "LIVE" : "EDIT", 149, 120, 1, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerUtilityFrame(uint16_t accent, const char* value, const char* tag, const char* rightText) {
  drawValueReadout(value, tag, accent);
  printRight(rightText, 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawSequencerGrid(14, 79, 132, 32, accent);
}

void drawSequencerLengthEdit(uint16_t accent) {
  char value[8];
  snprintf(value, sizeof(value), "%d", getSequencerLength());

  drawSequencerUtilityFrame(accent, value, "LEN", "steps");
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("active pattern");
  printRight(sequencerEnabled ? "LIVE" : "EDIT", 149, 120, 1, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerDirectionEdit(uint16_t accent) {
  drawSequencerUtilityFrame(accent, SEQUENCER_DIRECTION_NAMES[sequencerDirection], "DIR", "playback");
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("FWD REV PEND RND");
  printRight(sequencerEnabled ? "MOVE" : "DIR", 149, 120, 1, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerGateEdit(uint16_t accent) {
  char value[10];
  snprintf(value, sizeof(value), "%d%%", sequencerGate);

  drawSequencerUtilityFrame(accent, value, "GATE", "step hold");
  drawGrooveBar(50, 115, 60, 8, map(sequencerGate, 5, 100, 0, 58), accent);
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 112);
  tft.print("SHORT");
  printRight("LONG", 150, 112, 1, COL_MUTED);
}

void drawSequencerTransposeEdit(uint16_t accent) {
  char value[10];
  formatSequencerNote(value, sizeof(value), sequencerTranspose);

  drawSequencerUtilityFrame(accent, value, "TRN", "global");
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("base ");
  tft.print(frequency);
  tft.print("Hz");
  printRight("all steps", 149, 120, 1, COL_MUTED);
}

void drawSequencerStepEnableEdit(uint16_t accent) {
  drawSequencerUtilityFrame(accent, sequencerStepEnabled[sequencerSelectedStep] ? "ON" : "REST", "STEP", "mute");
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("step ");
  tft.print(sequencerSelectedStep + 1);
  printRight(sequencerStepEnabled[sequencerSelectedStep] ? "plays" : "silent", 149, 120, 1, sequencerStepEnabled[sequencerSelectedStep] ? COL_GOLD : COL_CORAL);
}

void drawSequencerRandomizeEdit(uint16_t accent) {
  drawSequencerUtilityFrame(accent, "RAND", "SEQ", "generate");
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("notes + rests");
  printRight("encoder", 149, 120, 1, COL_MUTED);
}

void drawSequencerClearEdit(uint16_t accent) {
  drawSequencerUtilityFrame(accent, "ZERO", "SEQ", "clear");
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("all notes 0st");
  printRight("encoder", 149, 120, 1, COL_MUTED);
}

void drawSequencerRotateEdit(uint16_t accent) {
  drawSequencerUtilityFrame(accent, "ROT", "SEQ", "shift");
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print("left / right");
  printRight("pattern", 149, 120, 1, COL_MUTED);
}

int getMixerVoiceLevel(int voiceIndex) {
  if (voiceIndex >= 0 && voiceIndex < WAVE_VOICE_COUNT) {
    return waveVoiceLevels[voiceIndex];
  }

  return kickLevel;
}

uint16_t getMixerVoiceColor(int voiceIndex, uint16_t accent) {
  if (voiceIndex == 0) return accent;
  if (voiceIndex == 1) return COL_SKY;
  if (voiceIndex == 2) return COL_NOTE;
  if (voiceIndex == 3) return COL_CORAL;
  return COL_GOLD;
}

void drawMixerLevelBars(uint16_t accent, int focusVoice) {
  const char* tags[] = { "V1", "V2", "V3", "V4", "K" };

  drawPanelFrame(10, 98, 140, 27, accent);

  for (int i = 0; i < 5; i++) {
    int level = getMixerVoiceLevel(i);
    int x = 17 + i * 27;
    int barH = map(level, 0, 100, 0, 15);
    uint16_t color = getMixerVoiceColor(i, accent);
    uint16_t frame = i == focusVoice ? color : COL_LINE;

    tft.drawRect(x, 102, 14, 16, frame);
    if (barH > 0) {
      tft.fillRect(x + 2, 116 - barH, 10, barH, color);
    }

    tft.setTextSize(1);
    tft.setTextColor(i == focusVoice ? color : COL_MUTED);
    tft.setCursor(x - 1, 119);
    tft.print(tags[i]);
  }
}

void drawMixerWaveEdit(int voice, uint16_t accent) {
  char value[10];
  snprintf(value, sizeof(value), "%d%%", waveVoiceLevels[voice]);

  drawValueReadout(value, menuItems[getMenuItemIndex(selectedItem)].tag, accent);
  printRight(waveformNames[waveVoiceShape[voice]], 150, 67, 1, getMixerVoiceColor(voice, accent));

  drawPanelFrame(10, 74, 140, 20, accent);
  drawWaveShapePreviewForParams(14, 78, 132, 12, waveVoiceShape[voice], (float)waveVoiceFold[voice], (float)waveVoicePhase[voice], getMixerVoiceColor(voice, accent));

  drawMixerLevelBars(accent, voice);
}

float getKickDesignerPreviewSample(float t) {
  t = clampFloat(t, 0.0f, 1.0f);
  float tail = 1.0f - t;
  float envelope = tail * tail * tail;
  float phaseCurve = t * (1.2f + kickPitch / 36.0f) + (1.0f - tail * tail) * (kickSweep / 95.0f);
  float click = t < 0.08f ? (1.0f - t / 0.08f) * (kickClick / 100.0f) * 0.55f : 0.0f;
  float sample = sinf(phaseCurve * TWO_PI_F) * envelope + click;
  float drive = 1.0f + kickDrive / 35.0f;

  sample *= drive;
  sample = (sample / (1.0f + fabsf(sample))) * 1.7f;
  return clampFloat(sample, -1.0f, 1.0f);
}

void drawKickDesignerTrace(int x, int y, int width, int height, uint16_t accent) {
  int centerY = y + height / 2;
  int prevY = centerY;

  for (int i = 0; i < width; i++) {
    float t = (float)i / (float)(width - 1);
    int nextY = centerY - (int)(getKickDesignerPreviewSample(t) * (height / 2 - 3));

    if (i > 0) {
      tft.drawLine(x + i - 1, prevY, x + i, nextY, accent);
    }

    prevY = nextY;
  }
}

void drawKickDesignerPanel(uint16_t accent) {
  drawPanelFrame(10, 74, 140, 34, accent);
  tft.drawFastHLine(14, 91, 132, rgb565(42, 47, 62));
  tft.drawFastVLine(14, 78, 26, rgb565(42, 47, 62));
  drawKickDesignerTrace(14, 78, 132, 26, accent);
}

void drawKickDesignerEdit(const char* tag, int value, int minValue, int maxValue, const char* leftLabel, const char* rightLabel, uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), tag, accent);
  printRight(kickLevel > 0 ? "armed" : "muted", 150, 67, 1, kickLevel > 0 ? COL_GOLD : COL_MUTED);
  drawKickDesignerPanel(accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 112);
  tft.print(leftLabel);
  printRight(rightLabel, 150, 112, 1, COL_MUTED);
  drawGrooveBar(50, 115, 60, 8, map(value, minValue, maxValue, 0, 58), accent);
}

void drawKickPitchEdit(uint16_t accent) {
  drawKickDesignerEdit("KPIT", kickPitch, 25, 120, "LOW", "HIGH", accent);
}

void drawKickDecayEdit(uint16_t accent) {
  drawKickDesignerEdit("KDEC", kickDecay, 80, 1200, "TIGHT", "LONG", accent);
}

void drawKickSweepEdit(uint16_t accent) {
  drawKickDesignerEdit("KSWP", kickSweep, 0, 260, "FLAT", "DROP", accent);
}

void drawKickClickEdit(uint16_t accent) {
  drawKickDesignerEdit("KCLK", kickClick, 0, 100, "SOFT", "CLICK", accent);
}

void drawKickDriveEdit(uint16_t accent) {
  drawKickDesignerEdit("KDRV", kickDrive, 0, 100, "CLEAN", "DRIVE", accent);
}

void drawMixerKickEdit(uint16_t accent) {
  char value[10];
  snprintf(value, sizeof(value), "%d%%", kickLevel);

  drawValueReadout(value, "KICK", accent);
  printRight(kickLevel > 0 ? "step kick" : "muted", 150, 67, 1, kickLevel > 0 ? COL_GOLD : COL_INK);

  drawPanelFrame(10, 74, 140, 20, accent);
  drawKickDesignerTrace(14, 78, 132, 12, accent);

  drawMixerLevelBars(accent, 4);
}

void drawBluetoothPowerEdit(uint16_t accent) {
  drawValueReadout(bluetoothEnabled ? "ON" : "OFF", "BT", accent);
  printRight(bluetoothEnabled ? getBluetoothStatusText() : "synth only", 150, 67, 1, bluetoothEnabled ? COL_INK : COL_GOLD);

  drawPanelFrame(10, 74, 140, 48, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 84);
  tft.print("Bluetooth radio");

  tft.setTextColor(bluetoothEnabled ? COL_NOTE : COL_GOLD);
  tft.setCursor(15, 98);
  tft.print(bluetoothEnabled ? "A2DP receiver enabled" : "disabled for wave test");

  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 114);
  tft.print(bluetoothEnabled ? BT_AUDIO_NAME : "PCM5102 stays on I2S");
}

void drawBluetoothPacketMeter(int x, int y, int width, int height, uint16_t accent) {
  bool streaming = isBluetoothStreaming();
  int bars = 12;
  int barW = width / bars;

  for (int i = 0; i < bars; i++) {
    int pulse = streaming ? (int)((btPacketCount + i * 5) % 11) : 0;
    int barH = streaming ? 4 + pulse : 2;
    barH = clampInt(barH, 2, height - 3);

    uint16_t color = streaming ? (i % 3 == 0 ? COL_GOLD : accent) : rgb565(54, 63, 79);
    int bx = x + i * barW + 1;
    int by = y + height - 2 - barH;
    int fillW = barW - 2;
    if (fillW < 1) fillW = 1;

    tft.fillRect(bx, by, fillW, barH, color);
  }
}

void drawBluetoothReceiverEdit(uint16_t accent) {
  bool streaming = isBluetoothStreaming();
  bool connected = bluetoothEnabled && btAudioStarted && a2dpSink.is_connected();
  const char* status = getBluetoothStatusText();

  drawValueReadout(status, "A2DP", accent);
  printRight(streaming ? "audio lock" : connected ? "connected" : "discoverable", 150, 67, 1, streaming ? COL_GOLD : COL_INK);

  drawPanelFrame(10, 74, 140, 34, accent);
  drawBluetoothPacketMeter(14, 79, 132, 25, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 112);
  tft.print("SBC");
  tft.setTextColor(COL_INK);
  tft.print(" -> PCM -> I2S");
  printRight(streaming ? "LIVE" : connected ? "LINK" : "PAIR", 149, 112, 1, streaming ? COL_GOLD : COL_MUTED);
}

void drawBluetoothDeviceEdit(uint16_t accent) {
  const char* stateText = "disabled";
  uint16_t stateColor = COL_GOLD;

  if (bluetoothEnabled) {
    stateText = btAudioStarted ? "visible" : "starting";
    stateColor = COL_INK;
  }

  drawValueReadout("PAIR", "NAME", accent);
  printRight(stateText, 150, 67, 1, stateColor);

  drawPanelFrame(10, 74, 140, 48, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 83);
  tft.print("device");

  tft.setTextColor(COL_INK);
  tft.setCursor(15, 96);
  tft.print(BT_AUDIO_NAME);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 112);
  tft.print("phone streams A2DP sink");
}

void drawBluetoothI2SEdit(uint16_t accent) {
  drawValueReadout("I2S", "DAC", accent);
  printRight("PCM5102", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 50, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 82);
  tft.print("BCK");
  tft.setTextColor(COL_INK);
  tft.print(" GPIO");
  tft.print(PCM5102_BCK);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(87, 82);
  tft.print("LCK");
  tft.setTextColor(COL_INK);
  tft.print(" GPIO");
  tft.print(PCM5102_LRCK);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 98);
  tft.print("DIN");
  tft.setTextColor(COL_INK);
  tft.print(" GPIO");
  tft.print(PCM5102_DIN);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(87, 98);
  tft.print("SCK");
  tft.setTextColor(COL_INK);
  tft.print(" GND");

  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 114);
  tft.print("3-wire I2S, 44.1k stereo");
}

void drawBluetoothOutputEdit(uint16_t accent) {
  bool streaming = isBluetoothStreaming();

  drawValueReadout("AUX", "OUT", accent);
  printRight(streaming ? "line active" : "line ready", 150, 67, 1, streaming ? COL_GOLD : COL_INK);

  drawPanelFrame(10, 74, 62, 48, accent);
  tft.drawCircle(30, 98, 12, streaming ? COL_GOLD : COL_LINE);
  tft.drawCircle(52, 98, 12, streaming ? accent : COL_LINE);
  tft.fillCircle(30, 98, 4, streaming ? COL_GOLD : COL_MUTED);
  tft.fillCircle(52, 98, 4, streaming ? accent : COL_MUTED);

  drawPanelFrame(82, 74, 68, 48, accent);
  drawBluetoothPacketMeter(87, 82, 58, 20, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(88, 109);
  tft.print("PCM5102 L/R");
}

void drawInputsEdit(uint16_t accent) {
  drawValueReadout("INPUT", "LIVE", accent);
  drawPanelFrame(10, 74, 140, 51, accent);

  bool encA = digitalRead(ENC_A) == LOW;
  bool encB = digitalRead(ENC_B) == LOW;
  bool pushDown = digitalRead(ENC_SW) == LOW;
  bool k0Down = digitalRead(BTN_K0) == LOW;

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 82);
  tft.print("A:");
  tft.setTextColor(encA ? COL_GOLD : COL_INK);
  tft.print(encA ? "LOW " : "HIGH");

  tft.setTextColor(COL_MUTED);
  tft.setCursor(82, 82);
  tft.print("B:");
  tft.setTextColor(encB ? COL_GOLD : COL_INK);
  tft.print(encB ? "LOW " : "HIGH");

  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 96);
  tft.print("P:");
  tft.setTextColor(pushDown ? COL_CORAL : COL_INK);
  tft.print(pushPressCount);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(82, 96);
  tft.print("K0:");
  tft.setTextColor(k0Down ? COL_CORAL : COL_INK);
  tft.print(k0PressCount);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 113);
  tft.print("steps ");
  tft.setTextColor(accent);
  tft.print(encoderUiStepsTotal);
}

void drawDisplayTestEdit(uint16_t accent) {
  drawValueReadout("160x128", "LCD", accent);
  drawPanelFrame(10, 74, 140, 51, accent);

  uint16_t swatches[] = {
    COL_NOTE,
    COL_GOLD,
    COL_CORAL,
    COL_VIOLET,
    COL_SKY,
    COL_INK
  };

  for (int i = 0; i < 6; i++) {
    tft.fillRect(15 + i * 22, 82, 18, 21, swatches[i]);
  }

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 114);
  tft.print("SPI 40MHz  ST7735");
}

void drawMiniKeyboard(int x, int y) {
  for (int i = 0; i < 7; i++) {
    tft.drawRect(x + i * 9, y, 9, 21, COL_INK);
  }

  int blackKeys[] = { 1, 2, 4, 5, 6 };

  for (int i = 0; i < 5; i++) {
    tft.fillRect(x + blackKeys[i] * 9 - 3, y, 6, 12, COL_HEADER);
    tft.drawRect(x + blackKeys[i] * 9 - 3, y, 6, 12, COL_LINE);
  }
}
