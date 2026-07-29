#pragma once

void drawBluetoothHeaderField(bool force) {
  uint32_t now = millis();

  if (!force && now - lastBtHeaderRefreshMs < BT_HEADER_REFRESH_MS) {
    return;
  }

  updateBluetoothRssi();

  bool connected = bluetoothEnabled && btAudioStarted && a2dpSink.is_connected();
  int bars = getBluetoothSignalBars();
  uint16_t statusColor = connected ? COL_SKY : COL_MUTED;

  if (!force && connected == lastBtHeaderConnected && bars == lastBtHeaderBars) {
    lastBtHeaderRefreshMs = now;
    return;
  }

  tft.fillRect(100, 3, 58, 14, COL_HEADER);

  const int bx = 115;
  tft.drawFastVLine(bx, 5, 11, statusColor);
  tft.drawLine(bx, 5, bx + 4, 8, statusColor);
  tft.drawLine(bx + 4, 8, bx, 10, statusColor);
  tft.drawLine(bx, 10, bx + 4, 12, statusColor);
  tft.drawLine(bx + 4, 12, bx, 15, statusColor);
  tft.drawLine(bx - 3, 7, bx + 3, 13, statusColor);
  tft.drawLine(bx - 3, 13, bx + 3, 7, statusColor);

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
  lastBtHeaderConnected = connected;
  lastBtHeaderBars = bars;
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

    case ACTION_TRANSPORT_MENU:
      tft.drawCircle(cx, cy, 9, color);
      tft.fillTriangle(cx - 4, cy - 5, cx - 4, cy + 5, cx + 3, cy, color);
      tft.fillRect(cx + 4, cy - 4, 3, 8, COL_GOLD);
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

    case ACTION_SYNTH_VIEW:
      tft.drawRect(cx - 9, cy - 7, 18, 14, color);
      tft.drawFastHLine(cx - 7, cy + 3, 14, COL_GOLD);
      tft.fillCircle(cx - 4, cy - 1, 2, color);
      tft.fillCircle(cx + 4, cy - 1, 2, COL_SKY);
      break;

    case ACTION_SEQ_BPM:
      tft.drawCircle(cx, cy, 8, color);
      tft.drawLine(cx, cy, cx + 5, cy - 5, color);
      tft.fillCircle(cx, cy, 2, COL_GOLD);
      break;

    case ACTION_SEQ_VOICE:
      tft.drawRect(cx - 8, cy - 6, 16, 12, color);
      tft.drawFastHLine(cx - 8, cy, 16, COL_LINE);
      tft.fillRect(cx - 6, cy - 4 + getActiveSequencerVoice() * 6, 12, 4, COL_GOLD);
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
    {
      int filledSegments = (sequencerLength * 4 + SEQUENCER_STEP_COUNT - 1) / SEQUENCER_STEP_COUNT;
      for (int i = 0; i < 4; i++) {
        tft.drawRect(cx - 8 + i * 5, cy - 5, 4, 10, i < filledSegments ? color : rgb565(62, 68, 82));
      }
      break;
    }

    case ACTION_SEQ_STEP_ENABLE:
      tft.drawCircle(cx, cy, 8, color);
      if (isSelectedSequencerStepEnabled()) {
        tft.fillCircle(cx, cy, 5, COL_GOLD);
      } else {
        tft.drawLine(cx - 6, cy + 6, cx + 6, cy - 6, COL_CORAL);
      }
      break;

    case ACTION_MIX_SINE:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[0], (float)waveVoiceFold[0], (float)waveVoicePhase[0], color);
      tft.drawFastHLine(cx - 8, cy + 7, map(waveVoiceLevels[0], 0, 100, 0, 16), COL_GOLD);
      break;

    case ACTION_MIX_SQUARE:
      drawWaveShapePreviewForParams(cx - 8, cy - 5, 16, 10, waveVoiceShape[1], (float)waveVoiceFold[1], (float)waveVoicePhase[1], color);
      tft.drawFastHLine(cx - 8, cy + 7, map(waveVoiceLevels[1], 0, 100, 0, 16), COL_GOLD);
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

    case ACTION_SURFACE_MODE:
      for (int i = 0; i < 4; i++) {
        int bx = cx - 9 + i * 5;
        tft.drawRect(bx, cy - 5, 4, 8, color);
        tft.drawCircle(bx + 2, cy + 7, 1, i == 3 ? COL_GOLD : color);
      }
      break;

    case ACTION_OLED_DEBUG:
      tft.drawRect(cx - 9, cy - 7, 18, 12, color);
      tft.drawFastHLine(cx - 6, cy + 8, 12, color);
      for (int i = 0; i < 4; i++) {
        tft.drawPixel(cx - 6 + i * 4, cy - 3, i == 3 ? COL_GOLD : color);
        tft.drawPixel(cx - 6 + i * 4, cy, i == 3 ? COL_GOLD : color);
      }
      break;

    case ACTION_MCP_DEBUG:
      tft.drawRect(cx - 7, cy - 7, 14, 14, color);
      for (int i = -5; i <= 5; i += 5) {
        tft.drawFastHLine(cx - 10, cy + i, 3, color);
        tft.drawFastHLine(cx + 8, cy + i, 3, color);
      }
      tft.fillCircle(cx, cy, 2, COL_GOLD);
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

void drawEffectEdit(MenuAction action, uint16_t accent) {
  int value = 0;
  int maximum = 100;
  const char* left = "DRY";
  const char* right = "WET";

  if (action == ACTION_DELAY_MIX) value = delayMix;
  if (action == ACTION_DELAY_TIME) {
    value = delayTimeMs;
    maximum = DELAY_MAX_MS;
    left = "SHORT";
    right = "LONG";
  }
  if (action == ACTION_DELAY_FEEDBACK) {
    value = delayFeedback;
    left = "ONE";
    right = "ECHO";
  }
  if (action == ACTION_REVERB_MIX) value = reverbMix;
  if (action == ACTION_REVERB_ROOM) {
    value = reverbRoom;
    left = "TIGHT";
    right = "HALL";
  }

  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), menuItems[getMenuItemIndex(selectedItem)].tag, accent);
  drawPanelFrame(10, 78, 140, 35, accent);
  drawGrooveBar(20, 91, 120, 10, map(value, 0, maximum, 0, 118), accent);
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 119);
  tft.print(left);
  printRight(right, 150, 119, 1, COL_MUTED);
}

void drawPresetEdit(MenuAction action, uint16_t accent) {
  char slotText[8];
  snprintf(slotText, sizeof(slotText), "P%d", presetSlot);
  drawValueReadout(slotText, "PRESET", accent);
  drawPanelFrame(10, 78, 140, 42, accent);
  tft.setTextSize(2);
  tft.setTextColor(action == ACTION_PRESET_SLOT ? accent : presetOperationOk ? COL_NOTE : COL_CORAL);
  tft.setCursor(20, 88);
  tft.print(action == ACTION_PRESET_SLOT ? "SELECT" : presetOperationText);
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 112);
  tft.print(action == ACTION_PRESET_SLOT ? "1..8 in flash" : "press to return");
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

void drawPitchEdit(uint16_t accent) {
  char pitchLabel[12];
  getPitchLabel(pitchLabel, sizeof(pitchLabel));

  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "PITCH", accent);
  printRight(pitchLabel, 150, 67, 1, COL_INK);
  drawPanelFrame(10, 74, 140, 41, accent);
  drawScopeGrid(10, 74, 140, 41);
  drawOscilloscopeTrace(14, 78, 132, 33, accent, false);
  printRight("native osc", 149, 120, 1, COL_MUTED);
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
  drawGrooveBar(14, 109, 132, 9, map(volume, 0, 100, 0, 130), accent);
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
  printRight(waveVoiceLevels[voice] > 0 ? "MIX" : "MUTE", 149, 120, 1, waveVoiceLevels[voice] > 0 ? COL_GOLD : COL_MUTED);
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
  drawValueReadout("SINE", "TEST", accent);
  printRight("DIRECT I2S", 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 42, accent);
  drawScopeGrid(10, 74, 140, 42);
  drawWaveShapePreviewForParams(14, 79, 132, 33, 0, 0.0f, 0.0f, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 120);
  tft.print(AUDIO_TEST_FREQUENCY_HZ);
  tft.print("Hz fixed");
  printRight(isSynthAudioActive() ? "I2S ON" : "I2S OFF", 149, 120, 1, isSynthAudioActive() ? COL_GOLD : COL_MUTED);
}

void drawSequencerGrid(int x, int y, int width, int height, uint16_t accent) {
  if (width <= 0 || height < 16) return;

  const int columns = 8;
  const int rows = 2;
  int voice = getActiveSequencerVoice();
  int length = getSequencerLength();
  int rowHeight = height / rows;
  uint16_t voiceColor = ITEM_COLORS[voice % ITEM_COLOR_COUNT];
  uint16_t disabledColor = rgb565(38, 43, 56);

  tft.fillRect(x, y, width, height, COL_BG);
  tft.drawFastHLine(x, y + rowHeight, width, COL_LINE);

  for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
    int row = step / columns;
    int column = step % columns;
    int left = (column * width) / columns;
    int right = ((column + 1) * width) / columns;
    int cellWidth = right - left;
    int drawX = x + left;
    int drawY = y + row * rowHeight;
    bool selected = step == sequencerSelectedStep;
    bool playing = transportRunning && sequencerEnabled && step == sequencerCurrentStep;
    bool inLength = step < length;
    bool enabled = isSequencerStepEnabled(voice, step);

    if (selected) {
      tft.fillRect(drawX + 1, drawY + 1, max(1, cellWidth - 2), max(1, rowHeight - 2), COL_PANEL);
      tft.drawRect(drawX + 1, drawY + 1, max(1, cellWidth - 2), max(1, rowHeight - 2), accent);
    }

    if (playing) {
      tft.fillRect(drawX + 2, drawY + 1, max(1, cellWidth - 4), 2, COL_GOLD);
    }

    if (enabled && inLength) {
      tft.fillRect(drawX + 3, drawY + rowHeight - 3, max(1, cellWidth - 6), 2, playing ? COL_GOLD : voiceColor);
    }

    char stepLabel[4];
    snprintf(stepLabel, sizeof(stepLabel), "%d", step + 1);
    int textWidth = strlen(stepLabel) * 6;
    uint16_t numberColor = !inLength ? disabledColor
                         : playing ? COL_GOLD
                         : selected ? accent
                         : enabled ? COL_INK
                         : COL_MUTED;
    tft.setTextSize(1);
    tft.setTextColor(numberColor);
    tft.setCursor(drawX + max(1, (cellWidth - textWidth) / 2), drawY + max(1, (rowHeight - 8) / 2));
    tft.print(stepLabel);

    if (column == 3) {
      tft.drawFastVLine(drawX + cellWidth - 1, drawY + 2, max(1, rowHeight - 4), COL_LINE);
    }
  }
}

void drawSynthViewVoiceMeters(int x, int y) {
  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    int level = waveVoiceLevels[i];
    int barH = map(level, 0, 100, 0, 18);
    int bx = x + i * 16;
    uint16_t color = ITEM_COLORS[i % ITEM_COLOR_COUNT];

    tft.drawRect(bx, y, 10, 20, COL_LINE);
    if (barH > 0) {
      tft.fillRect(bx + 2, y + 18 - barH, 6, barH, color);
    }

    tft.setTextSize(1);
    tft.setTextColor(level > 0 ? color : COL_MUTED);
    tft.setCursor(bx - 1, y + 22);
    tft.print("V");
    tft.print(i + 1);
  }
}

void drawSynthView(uint16_t accent, bool fullClear) {
  if (fullClear) {
    tft.fillScreen(COL_BG);
  }

  bool running = transportRunning && sequencerEnabled;
  uint16_t runColor = running ? COL_GOLD : COL_MUTED;
  int voice = getActiveSequencerVoice();
  int shownStep = running
    ? clampInt((int)sequencerCurrentStep, 0, getSequencerLength() - 1)
    : clampInt(sequencerSelectedStep, 0, getSequencerLength() - 1);
  bool stepEnabled = isSequencerStepEnabled(voice, shownStep);
  int previewShape = waveVoiceShape[voice];

  tft.fillRect(0, 0, SCREEN_W, 18, COL_BG);
  tft.drawFastHLine(0, 17, SCREEN_W, COL_LINE);
  tft.setTextSize(1);
  tft.setTextColor(accent);
  tft.setCursor(5, 5);
  tft.print("SYNTH");
  tft.fillCircle(50, 8, 3, runColor);
  tft.setTextColor(runColor);
  tft.setCursor(58, 5);
  tft.print(running ? "PLAY" : "STOP");

  char bpmText[12];
  snprintf(bpmText, sizeof(bpmText), "%d BPM", sequencerBpm);
  printRight(bpmText, 156, 5, 1, COL_INK);

  tft.fillRect(0, 19, SCREEN_W, 41, COL_BG);
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(5, 22);
  tft.print(running ? "PLAYHEAD" : "SELECTED");
  tft.setCursor(91, 22);
  tft.print("VOICE");

  char stepText[12];
  snprintf(stepText, sizeof(stepText), "%02d/%02d", shownStep + 1, getSequencerLength());
  tft.setTextSize(2);
  tft.setTextColor(stepEnabled ? accent : COL_MUTED);
  tft.setCursor(5, 32);
  tft.print(stepText);
  tft.setTextColor(ITEM_COLORS[voice % ITEM_COLOR_COUNT]);
  tft.setCursor(91, 32);
  tft.print("V");
  tft.print(voice + 1);

  tft.setTextSize(1);
  tft.setTextColor(stepEnabled ? COL_INK : COL_CORAL);
  tft.setCursor(5, 51);
  if (stepEnabled) {
    char noteText[10];
    snprintf(noteText, sizeof(noteText), "%+dst", getSequencerStepNote(voice, shownStep));
    tft.print(noteText);
  } else {
    tft.print("REST");
  }
  tft.setTextColor(COL_SKY);
  tft.setCursor(91, 51);
  tft.print(getWaveformShortName(previewShape));
  tft.setTextColor(COL_INK);
  tft.setCursor(121, 51);
  tft.print(frequency);
  tft.print("Hz");

  drawSequencerGrid(4, 62, 152, 36, accent);

  tft.fillRect(0, 101, SCREEN_W, 27, COL_BG);
  tft.drawFastHLine(4, 101, 152, COL_LINE);
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(4, 106);
  tft.print("L");
  tft.print(getSequencerLength());
  tft.print(" FWD");
  char voiceLevelText[12];
  snprintf(voiceLevelText, sizeof(voiceLevelText), "V1 %d", waveVoiceLevels[0]);
  printRight(voiceLevelText, 156, 106, 1, waveVoiceLevels[0] > 0 ? ITEM_COLORS[0] : COL_MUTED);

  tft.setCursor(4, 118);
  char liveControlValue[16];
  ControlSurface::formatLastParameterValue(liveControlValue, sizeof(liveControlValue));
  tft.setTextColor(accent);
  tft.print(ControlSurface::getLastParameterLabel());
  tft.print(" ");
  tft.setTextColor(COL_INK);
  tft.print(liveControlValue);
  snprintf(voiceLevelText, sizeof(voiceLevelText), "V2 %d", waveVoiceLevels[1]);
  printRight(voiceLevelText, 156, 118, 1, waveVoiceLevels[1] > 0 ? ITEM_COLORS[1] : COL_MUTED);
}

void formatSequencerNote(char* buffer, size_t size, int note) {
  snprintf(buffer, size, "%+dst", note);
}

void drawSequencerEditBody(const char* value, const char* tag, uint16_t accent) {
  tft.fillRect(0, 53, SCREEN_W, 62, COL_BG);
  tft.setTextSize(2);
  tft.setTextColor(accent);
  tft.setCursor(8, 56);
  tft.print(value);

  printRight(tag, 152, 61, 1, COL_MUTED);
  tft.drawFastHLine(8, 75, 144, COL_LINE);
  drawSequencerGrid(8, 79, 144, 34, accent);
}

void drawSequencerEditFooter(const char* left, const char* right,
                             uint16_t leftColor = COL_MUTED,
                             uint16_t rightColor = COL_MUTED) {
  tft.fillRect(0, 115, SCREEN_W, 13, COL_BG);
  tft.setTextSize(1);
  tft.setTextColor(leftColor);
  tft.setCursor(8, 119);
  tft.print(left);
  printRight(right, 152, 119, 1, rightColor);
}

void drawSequencerStartEdit(uint16_t accent) {
  drawSequencerEditBody("START", "TRANSPORT", accent);
  char stepText[12];
  snprintf(stepText, sizeof(stepText), "STEP %02d", sequencerCurrentStep + 1);
  drawSequencerEditFooter(stepText, sequencerEnabled ? "SEQ ON" : "SEQ OFF",
                          COL_MUTED, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerStopEdit(uint16_t accent) {
  drawSequencerEditBody("STOP", "TRANSPORT", accent);
  drawSequencerEditFooter("TRANSPORT", transportRunning ? "PLAY" : "SILENT",
                          COL_MUTED, transportRunning ? COL_MUTED : COL_GOLD);
}

void drawSequencerRunEdit(uint16_t accent) {
  char bpmText[12];
  snprintf(bpmText, sizeof(bpmText), "%d BPM", sequencerBpm);
  drawSequencerEditBody(bpmText, transportRunning && sequencerEnabled ? "RUN" : "STOP", accent);
  char stepText[16];
  snprintf(stepText, sizeof(stepText), "STEP %02d/%02d", sequencerCurrentStep + 1, getSequencerLength());
  char lengthText[12];
  snprintf(lengthText, sizeof(lengthText), "LEN %d", getSequencerLength());
  drawSequencerEditFooter(stepText, lengthText);
}

void drawSequencerBpmEdit(uint16_t accent) {
  char value[12];
  snprintf(value, sizeof(value), "%d", sequencerBpm);
  drawSequencerEditBody(value, "BPM", accent);
  drawSequencerEditFooter("1/16 NOTE", sequencerEnabled ? "SEQ ON" : "SEQ OFF",
                          COL_MUTED, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerVoiceEdit(uint16_t accent) {
  char value[8];
  int voice = getActiveSequencerVoice();
  snprintf(value, sizeof(value), "V%d", voice + 1);

  drawSequencerEditBody(value, "TRACK", accent);
  drawSequencerEditFooter(getWaveformShortName(waveVoiceShape[voice]), "16 KEYS",
                          ITEM_COLORS[voice % ITEM_COLOR_COUNT], COL_MUTED);
}

void drawSequencerStepEdit(uint16_t accent) {
  char value[8];
  snprintf(value, sizeof(value), "S%d", sequencerSelectedStep + 1);

  drawSequencerEditBody(value, "SELECT", accent);

  char noteText[10];
  formatSequencerNote(noteText, sizeof(noteText), getSelectedSequencerStepNote());

  char leftText[18];
  snprintf(leftText, sizeof(leftText), "V%d  %s", getActiveSequencerVoice() + 1, noteText);
  drawSequencerEditFooter(leftText, isSelectedSequencerStepEnabled() ? "ON" : "REST",
                          COL_MUTED, isSelectedSequencerStepEnabled() ? COL_GOLD : COL_CORAL);
}

void drawSequencerNoteEdit(uint16_t accent) {
  char value[10];
  formatSequencerNote(value, sizeof(value), getSelectedSequencerStepNote());

  drawSequencerEditBody(value, "SEMITONE", accent);
  char stepText[12];
  snprintf(stepText, sizeof(stepText), "V%d  S%02d", getActiveSequencerVoice() + 1, sequencerSelectedStep + 1);
  drawSequencerEditFooter(stepText, sequencerEnabled ? "LIVE" : "EDIT",
                          COL_MUTED, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerUtilityFrame(uint16_t accent, const char* value, const char* tag, const char* rightText) {
  (void)rightText;
  drawSequencerEditBody(value, tag, accent);
}

void drawSequencerLengthEdit(uint16_t accent) {
  char value[8];
  snprintf(value, sizeof(value), "%d", getSequencerLength());

  drawSequencerUtilityFrame(accent, value, "LENGTH", "STEPS");
  drawSequencerEditFooter("PATTERN", sequencerEnabled ? "LIVE" : "EDIT",
                          COL_MUTED, sequencerEnabled ? COL_GOLD : COL_MUTED);
}

void drawSequencerStepEnableEdit(uint16_t accent) {
  bool enabled = isSelectedSequencerStepEnabled();
  drawSequencerUtilityFrame(accent, enabled ? "ON" : "REST", "STEP", "MUTE");
  char stepText[12];
  snprintf(stepText, sizeof(stepText), "V%d  S%02d", getActiveSequencerVoice() + 1, sequencerSelectedStep + 1);
  drawSequencerEditFooter(stepText, enabled ? "ACTIVE" : "MUTED",
                          COL_MUTED, enabled ? COL_GOLD : COL_CORAL);
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
  return COL_GOLD;
}

void drawMixerLevelBars(uint16_t accent, int focusVoice) {
  drawPanelFrame(10, 98, 140, 27, accent);

  int meterCount = WAVE_VOICE_COUNT + 1;
  int spacing = 120 / meterCount;

  for (int i = 0; i < meterCount; i++) {
    bool isKickMeter = i == WAVE_VOICE_COUNT;
    int level = isKickMeter ? kickLevel : getMixerVoiceLevel(i);
    int x = 22 + i * spacing;
    int barH = map(level, 0, 100, 0, 15);
    uint16_t color = isKickMeter ? COL_GOLD : getMixerVoiceColor(i, accent);
    uint16_t frame = i == focusVoice || (isKickMeter && focusVoice == WAVE_VOICE_COUNT) ? color : COL_LINE;

    tft.drawRect(x, 102, 14, 16, frame);
    if (barH > 0) {
      tft.fillRect(x + 2, 116 - barH, 10, barH, color);
    }

    tft.setTextSize(1);
    tft.setTextColor(frame == color ? color : COL_MUTED);
    tft.setCursor(x - 1, 119);
    if (isKickMeter) {
      tft.print("K");
    } else {
      tft.print("V");
      tft.print(i + 1);
    }
  }
}

void drawMixerWaveEdit(int voice, uint16_t accent) {
  if (voice < 0 || voice >= WAVE_VOICE_COUNT) return;

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

  drawMixerLevelBars(accent, WAVE_VOICE_COUNT);
}

void drawBluetoothPowerEdit(uint16_t accent) {
  drawValueReadout(bluetoothEnabled ? "ON" : "OFF", "BT", accent);
  printRight(bluetoothEnabled ? "ENABLED" : "DISABLED", 150, 67, 1, bluetoothEnabled ? COL_SKY : COL_MUTED);

  drawPanelFrame(10, 74, 140, 48, accent);

  tft.setTextSize(1);
  tft.setTextColor(bluetoothEnabled ? COL_SKY : COL_MUTED);
  tft.setCursor(16, 86);
  tft.print("Bluetooth receiver");
  tft.setTextColor(COL_INK);
  tft.setCursor(16, 101);
  tft.print(bluetoothEnabled ? "Pairing ready" : "Radio off");
  tft.setTextColor(COL_MUTED);
  tft.setCursor(16, 115);
  tft.print("Rotate toggles");
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
  printRight(streaming ? "STREAMING" : connected ? "CONNECTED" : bluetoothEnabled ? "WAITING" : "OFF", 150, 67, 1, streaming ? COL_GOLD : connected ? COL_SKY : COL_MUTED);

  drawPanelFrame(10, 74, 140, 34, accent);
  drawBluetoothPacketMeter(14, 79, 132, 25, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(11, 112);
  tft.print(BT_AUDIO_NAME);
}

void drawBluetoothDeviceEdit(uint16_t accent) {
  const char* stateText = "OFF";
  uint16_t stateColor = COL_MUTED;

  if (bluetoothEnabled) {
    stateText = btAudioStarted ? "VISIBLE" : "STARTING";
    stateColor = COL_SKY;
  }

  drawValueReadout("NAME", "BT", accent);
  printRight(stateText, 150, 67, 1, stateColor);

  drawPanelFrame(10, 74, 140, 48, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 84);
  tft.print("Device name");

  tft.setTextColor(COL_INK);
  tft.setCursor(15, 99);
  tft.print(BT_AUDIO_NAME);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 115);
  tft.print("Pair from phone/PC");
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
  tft.print(" ");
  tft.print(PCM5102_BCK_LABEL);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(87, 82);
  tft.print("LCK");
  tft.setTextColor(COL_INK);
  tft.print(" ");
  tft.print(PCM5102_LCK_LABEL);

  tft.setTextColor(COL_MUTED);
  tft.setCursor(15, 98);
  tft.print("DIN");
  tft.setTextColor(COL_INK);
  tft.print(" ");
  tft.print(PCM5102_DIN_LABEL);

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

  drawValueReadout("I2S", "OUT", accent);
  printRight(streaming ? "ACTIVE" : "READY", 150, 67, 1, streaming ? COL_GOLD : COL_SKY);

  drawPanelFrame(10, 74, 140, 48, accent);
  drawBluetoothPacketMeter(16, 80, 128, 20, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(16, 108);
  tft.print("PCM5102  44.1 kHz");
  tft.setCursor(16, 117);
  tft.print("SCK / BCK / DIN / LCK");
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

void drawHardwareTestReadout(const char* value, const char* tag, uint16_t accent) {
  uint8_t valueSize = strlen(value) > 8 ? 1 : 2;
  tft.setTextSize(valueSize);
  tft.setTextColor(accent);
  tft.setCursor(10, valueSize == 2 ? 56 : 60);
  tft.print(value);
  printRight(tag, 150, 59, 1, COL_MUTED);
}

uint8_t countReadyOleds(uint8_t mask) {
  uint8_t count = 0;
  for (uint8_t channel = 0; channel < 4; channel++) {
    if (mask & (1U << channel)) count++;
  }
  return count;
}

void drawHardwareStatusLabel(int x, int y, const char* label, bool ready) {
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(x, y);
  tft.print(label);
  tft.setTextColor(ready ? COL_NOTE : COL_CORAL);
  tft.print(ready ? "+" : "-");
}

void drawControlSurfaceEdit(uint16_t accent) {
  drawHardwareTestReadout(ControlSurface::getSynthPageName(), transportRunning ? "LIVE PLAY" : "LIVE STOP", accent);

  char liveValue[18];
  ControlSurface::formatLastParameterValue(liveValue, sizeof(liveValue));
  tft.fillRect(8, 75, 144, 28, COL_BG);
  tft.drawFastHLine(8, 75, 144, COL_LINE);
  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 81);
  tft.print(ControlSurface::getLastParameterLabel());
  tft.setTextSize(strlen(liveValue) <= 8 ? 2 : 1);
  tft.setTextColor(accent);
  tft.setCursor(10, 91);
  tft.print(liveValue);

  tft.drawFastHLine(8, 106, 144, COL_LINE);
  tft.setTextSize(1);
  tft.setTextColor(COL_INK);
  tft.setCursor(10, 111);
  tft.print("E1-E3 PARAM");
  printRight("E4 PAGE", 150, 111, 1, COL_GOLD);
  drawHardwareStatusLabel(10, 118, "OLED", ControlSurface::getOledReadyMask() == 0x0F);
  drawHardwareStatusLabel(62, 118, "MCP", ControlSurface::isEncoderExpanderReady());
}

void printOledDebugAddress(uint8_t channel) {
  uint8_t address = ControlSurface::getOledChannelAddress(channel);
  if (address == 0) {
    tft.print("--");
    return;
  }
  if (address < 0x10) tft.print('0');
  tft.print(address, HEX);
}

void drawOledDebugEdit(uint16_t accent) {
  char oledSummary[16];
  snprintf(oledSummary, sizeof(oledSummary), "OLED %u/4", countReadyOleds(ControlSurface::getOledReadyMask()));
  drawHardwareTestReadout(oledSummary, "D21/D22", accent);
  drawPanelFrame(10, 74, 140, 51, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 80);
  tft.print("SDA21:");
  tft.setTextColor(ControlSurface::isMainSdaHigh() ? COL_NOTE : COL_CORAL);
  tft.print(ControlSurface::isMainSdaHigh() ? "H" : "L");
  tft.setTextColor(COL_MUTED);
  tft.print(" SCL22:");
  tft.setTextColor(ControlSurface::isMainSclHigh() ? COL_NOTE : COL_CORAL);
  tft.print(ControlSurface::isMainSclHigh() ? "H" : "L");
  tft.setTextColor(COL_MUTED);
  tft.print(" BUS:");
  tft.print(ControlSurface::getMainBusDeviceCount());

  for (uint8_t channel = 0; channel < 4; channel++) {
    int x = 14 + channel * 33;
    bool ready = (ControlSurface::getOledReadyMask() & (1U << channel)) != 0;
    tft.fillRect(x, 91, 29, 14, COL_BG);
    tft.drawRect(x, 91, 29, 14, ready ? COL_NOTE : COL_CORAL);
    tft.setTextColor(ready ? COL_NOTE : COL_CORAL);
    tft.setCursor(x + 2, 94);
    tft.print(channel + 1);
    tft.print(':');
    printOledDebugAddress(channel);
  }

  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 108);
  tft.print("PCA:");
  if (ControlSurface::isPcaReady()) tft.print(ControlSurface::getPcaAddress(), HEX);
  else tft.print("--");
  tft.print(" ACK:");
  tft.print(ControlSurface::getOledDetectedMask(), HEX);
  tft.print(" INIT:");
  tft.print(ControlSurface::getOledReadyMask(), HEX);

  tft.setCursor(14, 117);
  tft.print("K ");
  for (uint8_t address = 0x30; address <= 0x33; address++) {
    tft.print(address, HEX);
    tft.print(ControlSurface::isNeoKeyAddressPresent(address) ? "+" : "-");
  }
  printRight("SCAN", 146, 117, 1, COL_MUTED);
}

void printMcpEncoderTriplet(uint8_t encoderIndex) {
  if (!ControlSurface::isMcpRawPortsValid()) {
    tft.print("---");
    return;
  }
  for (uint8_t signal = 0; signal < 3; signal++) {
    tft.print(ControlSurface::getEncoderPinHigh(encoderIndex, signal) ? 'H' : 'L');
  }
}

void drawMcpDebugEdit(uint16_t accent) {
  drawHardwareTestReadout(ControlSurface::isEncoderExpanderReady() ? "MCP OK" : "MCP MISS",
                          "D19/D15", accent);
  drawPanelFrame(10, 74, 140, 51, accent);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(14, 80);
  tft.print("SDA19:");
  tft.setTextColor(ControlSurface::isEncoderSdaHigh() ? COL_NOTE : COL_CORAL);
  tft.print(ControlSurface::isEncoderSdaHigh() ? "H" : "L");
  tft.setTextColor(COL_MUTED);
  tft.print(" SCL15:");
  tft.setTextColor(ControlSurface::isEncoderSclHigh() ? COL_NOTE : COL_CORAL);
  tft.print(ControlSurface::isEncoderSclHigh() ? "H" : "L");
  tft.setTextColor(COL_MUTED);
  tft.print(" BUS:");
  tft.print(ControlSurface::getEncoderBusDeviceCount());

  tft.setCursor(14, 89);
  tft.print("ADDR:");
  tft.print(ControlSurface::isEncoderBusAddressPresent(0x20) ? "20" : "--");
  tft.print(" RAW:");
  if (ControlSurface::isMcpRawPortsValid()) {
    uint16_t raw = ControlSurface::getMcpRawPorts();
    if (raw < 0x1000) tft.print('0');
    if (raw < 0x0100) tft.print('0');
    if (raw < 0x0010) tft.print('0');
    tft.print(raw, HEX);
  } else {
    tft.print("----");
  }

  for (uint8_t encoder = 0; encoder < 4; encoder++) {
    int x = 14 + encoder * 33;
    bool valid = ControlSurface::isMcpRawPortsValid();
    tft.fillRect(x, 99, 29, 22, COL_BG);
    tft.drawRect(x, 99, 29, 22, valid ? accent : COL_CORAL);
    tft.setTextColor(valid ? accent : COL_CORAL);
    tft.setCursor(x + 2, 102);
    tft.print('E');
    tft.print(encoder + 1);
    tft.setTextColor(COL_INK);
    tft.setCursor(x + 16, 102);
    tft.print(ControlSurface::getEncoderCount(encoder));
    tft.setTextColor(COL_MUTED);
    tft.setCursor(x + 5, 112);
    printMcpEncoderTriplet(encoder);
  }
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
