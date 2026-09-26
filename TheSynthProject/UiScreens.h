#pragma once

// =======================
// BOOT / PREMENU DRAW
// =======================

void drawBootScreen() {
  uint32_t elapsed = millis() - bootStartMs;
  int progress = bootReady ? 100 : clampInt((int)((elapsed * 100) / bootDurationMs), 0, 100);

  if (fullRedraw || lastDrawnMode != MODE_BOOT) {
    tft.fillScreen(COL_BG);

    tft.drawFastHLine(10, 19, 140, COL_LINE);
    tft.drawFastHLine(10, 101, 140, COL_LINE);

    tft.setTextColor(COL_NOTE);
    tft.setTextSize(2);
    tft.setCursor(8, 30);
    tft.print("Polyrhythmic");

    tft.setTextColor(COL_GOLD);
    tft.setCursor(45, 52);
    tft.print("Glider");

    tft.setTextSize(1);
    tft.setTextColor(COL_MUTED);
    tft.setCursor(12, 78);
    tft.print("made with love and weed");

    tft.drawRect(18, 108, 124, 8, COL_LINE);

    lastBootProgress = -1;
    lastDrawnMode = MODE_BOOT;
    fullRedraw = false;
  }

  if (progress != lastBootProgress) {
    int fillW = map(progress, 0, 100, 0, 122);

    tft.fillRect(19, 109, 122, 6, COL_PANEL);
    if (fillW > 0) {
      tft.fillRect(19, 109, fillW, 6, COL_NOTE);
    }

    lastBootProgress = progress;
  }
  tft.fillRect(0, 119, SCREEN_W, 9, COL_BG);
  if (!DaisyBridge::isConnected()) {
    tft.setTextSize(1);
    tft.setTextColor(COL_CORAL);
    tft.setCursor(30, 120);
    tft.print("waiting daisy :)");
  }
}

void drawPreMenuBase() {
  if (TYPE_COUNT <= VISIBLE_TYPE_ROWS) topType = 0;

  drawStageBackdrop();

  drawHeader("SELECT AREA");

  lastSelectedType = -1;
  lastTopType = -1;
}

void drawTypeRow(int row, int typeIndex) {
  int y = 21 + row * 18;
  bool selected = typeIndex == selectedType;
  bool active = typeIndex == activeType;
  uint16_t accent = getAccentColor(typeIndex);
  uint16_t bg = selected ? COL_PANEL : COL_BG;

  tft.fillRect(0, y, SCREEN_W, 18, bg);

  if (selected) {
    tft.fillTriangle(0, y, 0, y + 17, 8, y + 8, accent);
    tft.drawFastVLine(154, y + 2, 14, accent);
    tft.drawFastHLine(12, y + 17, 136, accent);
  } else {
    tft.drawFastHLine(12, y + 17, 136, rgb565(18, 23, 36));
  }

  tft.setTextSize(1);
  tft.setTextColor(selected ? COL_INK : COL_MUTED);
  tft.setCursor(18, y + 6);
  tft.print(areaChoices[typeIndex].label);

  (void)active;
}

void drawVisibleTypeRows() {
  bool allAreasVisible = TYPE_COUNT <= VISIBLE_TYPE_ROWS;

  for (int row = 0; row < VISIBLE_TYPE_ROWS; row++) {
    if (row >= TYPE_COUNT) {
      tft.fillRect(0, 21 + row * 18, SCREEN_W, 18, COL_BG);
    } else {
      // A complete list is indexed directly by row. It must never inherit a
      // circular scroll offset from a previous selection at the list edge.
      int typeIndex = allAreasVisible ? row : wrapIndex(topType + row, TYPE_COUNT);
      drawTypeRow(row, typeIndex);
    }
  }

  lastSelectedType = selectedType;
  lastTopType = topType;
}

void updatePreMenuFast() {
  bool allAreasVisible = TYPE_COUNT <= VISIBLE_TYPE_ROWS;
  if (allAreasVisible) topType = 0;

  if (fullRedraw || lastDrawnMode != MODE_PREMENU) {
    drawPreMenuBase();

    drawVisibleTypeRows();

    lastDrawnMode = MODE_PREMENU;
    fullRedraw = false;
    return;
  }

  if (topType != lastTopType) {
    drawVisibleTypeRows();
    return;
  }

  if (selectedType != lastSelectedType) {
    int oldRow = allAreasVisible ? lastSelectedType : wrapIndex(lastSelectedType - topType, TYPE_COUNT);
    int newRow = allAreasVisible ? selectedType : wrapIndex(selectedType - topType, TYPE_COUNT);
    if (oldRow < VISIBLE_TYPE_ROWS) drawTypeRow(oldRow, lastSelectedType);
    if (newRow < VISIBLE_TYPE_ROWS) drawTypeRow(newRow, selectedType);
    lastSelectedType = selectedType;
  }
}

// =======================
// MENU DRAW
// =======================

void drawMenuBase() {
  drawStageBackdrop();

  drawHeader(getCurrentMenuTitle());

  lastSelectedItem = -1;
  lastTopItem = -1;
}

void drawMenuRow(int row, int visibleIndex) {
  int itemIndex = getMenuItemIndex(visibleIndex);
  int y = 25 + row * 20;
  bool selected = visibleIndex == selectedItem;
  uint16_t accent = getAccentColor(itemIndex);
  uint16_t rowBg = selected ? COL_PANEL : COL_BG;

  tft.fillRect(0, y - 2, SCREEN_W, 22, rowBg);

  if (selected) {
    tft.fillTriangle(0, y - 2, 0, y + 18, 9, y + 8, accent);
    tft.drawFastVLine(154, y, 16, accent);
    tft.fillCircle(18, y + 8, 7, accent);
    tft.drawCircle(18, y + 8, 9, COL_GOLD);
    drawParamIcon(itemIndex, 18, y + 8, COL_BG);
  } else {
    drawParamIcon(itemIndex, 18, y + 8, COL_MUTED);
  }

  tft.setTextSize(1);
  tft.setTextColor(selected ? COL_INK : COL_MUTED);
  tft.setCursor(34, y + 6);
  tft.print(menuItems[itemIndex].label);

  if (selected) {
    tft.drawFastHLine(34, y + 17, 110, accent);
  }
}

void drawVisibleMenuRows() {
  int count = getActiveMenuCount();

  for (int row = 0; row < VISIBLE_ROWS; row++) {
    int visibleIndex = count > 0 ? wrapIndex(topItem + row, count) : 0;

    if (row >= count) {
      tft.fillRect(0, 23 + row * 20, SCREEN_W, 22, COL_BG);
    } else {
      drawMenuRow(row, visibleIndex);
    }
  }

  lastSelectedItem = selectedItem;
  lastTopItem = topItem;
}

void updateMenuFast() {
  if (fullRedraw || lastDrawnMode != MODE_MENU) {
    drawMenuBase();
    drawVisibleMenuRows();

    fullRedraw = false;
    lastDrawnMode = MODE_MENU;
    return;
  }

  if (topItem != lastTopItem) {
    drawVisibleMenuRows();
    return;
  }

  if (selectedItem != lastSelectedItem) {
    int count = getActiveMenuCount();
    int oldRow = wrapIndex(lastSelectedItem - topItem, count);
    int newRow = wrapIndex(selectedItem - topItem, count);

    if (oldRow >= 0 && oldRow < VISIBLE_ROWS) {
      drawMenuRow(oldRow, lastSelectedItem);
    }

    if (newRow >= 0 && newRow < VISIBLE_ROWS) {
      drawMenuRow(newRow, selectedItem);
    }

    lastSelectedItem = selectedItem;
  }
}

// =======================
// EDIT DRAW
// =======================

void drawEditBase() {
  int itemIndex = getMenuItemIndex(selectedItem);
  uint16_t accent = getAccentColor(itemIndex);

  drawStageBackdrop();
  drawHeader("LIVE EDIT");

  tft.fillCircle(18, 37, 10, accent);
  drawParamIcon(itemIndex, 18, 37, COL_BG);

  tft.setTextSize(1);
  tft.setTextColor(accent);
  tft.setCursor(34, 28);
  tft.print(menuItems[itemIndex].tag);

  tft.setTextColor(COL_INK);
  tft.setCursor(34, 39);
  tft.print(menuItems[itemIndex].label);

  tft.drawFastHLine(8, 51, 144, COL_LINE);

  lastWaveform = -1;
  lastFrequency = -1;
  lastVolume = -1;
  lastAttack = -1;
  lastDecay = -1;
  lastSustain = -1;
  lastReleaseMs = -1;
  lastWaveFold = -1;
  lastWavePhase = -1;
  lastFilterEnabled = -1;
  lastFilterType = -1;
  lastFilterCutoff = -1;
  lastDelayMix = -1;
  lastDelayTimeMs = -1;
  lastDelayFeedback = -1;
  lastReverbMix = -1;
  lastReverbRoom = -1;
  lastPresetSlot = -1;
  lastSurfaceMode = -1;
  lastSurfacePage = -1;
  lastMcpRawPorts = 0;
  for (uint8_t i = 0; i < 4; i++) lastMcpEncoderCount[i] = 0;
  lastWaveVoiceParamChecksum = -1;
  lastTransportRunning = -1;
  lastSequencerEnabled = -1;
  lastSequencerBpm = -1;
  lastSequencerLength = -1;
  lastSequencerVoice = -1;
  lastSequencerTrack = -1;
  lastSequencerSelectedStep = -1;
  lastSequencerCurrentStep = -1;
  lastDaisyCursorSynchronized = -1;
  lastDaisyConnected = -1;
  lastSequencerPage = -1;
  lastSequencerAutoScroll = -1;
  lastSequencerNoteChecksum = -1;
  lastWaveVoiceChecksum = -1;
  lastKickLevel = -1;
  lastKickParamChecksum = -1;

  valueDirty = true;
}

bool actionShowsSequencerPlayhead(MenuAction action) {
  switch (action) {
    case ACTION_SYNTH_VIEW:
    case ACTION_SEQ_PATTERN_MENU:
    case ACTION_SEQ_START:
    case ACTION_SEQ_STOP:
    case ACTION_SEQ_BPM:
    case ACTION_SEQ_STEP:
    case ACTION_SEQ_NOTE:
    case ACTION_SEQ_LENGTH:
    case ACTION_SEQ_PAGE:
    case ACTION_SEQ_AUTO_SCROLL:
    case ACTION_SEQ_STEP_ENABLE:
      return true;

    default:
      return false;
  }
}

bool editValueChanged() {
  MenuAction action = getSelectedAction();
  if (waveform != lastWaveform) return true;
  if (frequency != lastFrequency) return true;
  if (volume != lastVolume) return true;
  if (attack != lastAttack) return true;
  if (decay != lastDecay) return true;
  if (sustain != lastSustain) return true;
  if (releaseMs != lastReleaseMs) return true;
  if (waveFold != lastWaveFold) return true;
  if (wavePhase != lastWavePhase) return true;
  if ((int)filterEnabled != lastFilterEnabled) return true;
  if (filterType != lastFilterType) return true;
  if (filterCutoff != lastFilterCutoff) return true;
  if (delayMix != lastDelayMix) return true;
  if (delayTimeMs != lastDelayTimeMs) return true;
  if (delayFeedback != lastDelayFeedback) return true;
  if (reverbMix != lastReverbMix) return true;
  if (reverbRoom != lastReverbRoom) return true;
  if (presetSlot != lastPresetSlot) return true;
  if ((int)ControlSurface::getMode() != lastSurfaceMode) return true;
  if ((int)ControlSurface::getSynthPage() != lastSurfacePage) return true;
  if (action == ACTION_MCP_DEBUG) {
    if (ControlSurface::getMcpRawPorts() != lastMcpRawPorts) return true;
    for (uint8_t i = 0; i < 4; i++) {
      if (ControlSurface::getEncoderCount(i) != lastMcpEncoderCount[i]) return true;
    }
  }
  if (getWaveVoiceParamChecksum() != lastWaveVoiceParamChecksum) return true;
  if ((int)transportRunning != lastTransportRunning) return true;
  if ((int)sequencerEnabled != lastSequencerEnabled) return true;
  if (sequencerBpm != lastSequencerBpm) return true;
  if (sequencerLength != lastSequencerLength) return true;
  if (activeSequencerVoice != lastSequencerVoice) return true;
  if (activeSequencerTrack != lastSequencerTrack) return true;
  if (sequencerSelectedStep != lastSequencerSelectedStep) return true;
  if (getSequencerPage() != lastSequencerPage) return true;
  if ((int)sequencerAutoScroll != lastSequencerAutoScroll) return true;
  if ((DaisyBridge::isConnected() ? 1 : 0) != lastDaisyConnected) return true;
  // Ordinary synth, ADSR, FX and utility pages do not display the playhead.
  // Avoid clearing and repainting them on every sixteenth note.
  if (actionShowsSequencerPlayhead(action) &&
      sequencerCurrentStep != lastSequencerCurrentStep) return true;
  if (getSequencerNoteChecksum() != lastSequencerNoteChecksum) return true;
  if (getLevelChecksum(waveVoiceLevels, WAVE_VOICE_COUNT) != lastWaveVoiceChecksum) return true;
  if (kickLevel != lastKickLevel) return true;
  if (getKickParamChecksum() != lastKickParamChecksum) return true;

  return false;
}

void saveLastValues() {
  lastWaveform = waveform;
  lastFrequency = frequency;
  lastVolume = volume;
  lastAttack = attack;
  lastDecay = decay;
  lastSustain = sustain;
  lastReleaseMs = releaseMs;
  lastWaveFold = waveFold;
  lastWavePhase = wavePhase;
  lastFilterEnabled = (int)filterEnabled;
  lastFilterType = filterType;
  lastFilterCutoff = filterCutoff;
  lastDelayMix = delayMix;
  lastDelayTimeMs = delayTimeMs;
  lastDelayFeedback = delayFeedback;
  lastReverbMix = reverbMix;
  lastReverbRoom = reverbRoom;
  lastPresetSlot = presetSlot;
  lastSurfaceMode = (int)ControlSurface::getMode();
  lastSurfacePage = (int)ControlSurface::getSynthPage();
  lastMcpRawPorts = ControlSurface::getMcpRawPorts();
  for (uint8_t i = 0; i < 4; i++) {
    lastMcpEncoderCount[i] = ControlSurface::getEncoderCount(i);
  }
  lastWaveVoiceParamChecksum = getWaveVoiceParamChecksum();
  lastTransportRunning = (int)transportRunning;
  lastSequencerEnabled = (int)sequencerEnabled;
  lastSequencerBpm = sequencerBpm;
  lastSequencerLength = sequencerLength;
  lastSequencerVoice = activeSequencerVoice;
  lastSequencerTrack = activeSequencerTrack;
  lastSequencerSelectedStep = sequencerSelectedStep;
  lastSequencerCurrentStep = sequencerCurrentStep;
  lastDaisyCursorSynchronized = DaisyBridge::isCursorSynchronized() ? 1 : 0;
  lastDaisyConnected = DaisyBridge::isConnected() ? 1 : 0;
  lastSequencerPage = getSequencerPage();
  lastSequencerAutoScroll = (int)sequencerAutoScroll;
  lastSequencerNoteChecksum = getSequencerNoteChecksum();
  lastWaveVoiceChecksum = getLevelChecksum(waveVoiceLevels, WAVE_VOICE_COUNT);
  lastKickLevel = kickLevel;
  lastKickParamChecksum = getKickParamChecksum();
}

void drawEditValue() {
  static uint32_t lastInputsDrawMs = 0;
  MenuAction action = getSelectedAction();
  bool clearBody = valueDirty;
  if (action == ACTION_INPUTS) {
    // The input monitor is live, but an uncapped repaint saturates the TFT bus.
    // Ten frames per second is responsive and leaves ample time for controls.
    uint32_t now = millis();
    if (!valueDirty && now - lastInputsDrawMs < 100) return;
    lastInputsDrawMs = now;
  } else {
    if (!valueDirty && !editValueChanged()) return;
  }

  int itemIndex = getMenuItemIndex(selectedItem);
  uint16_t accent = getAccentColor(itemIndex);

  valueDirty = false;
  saveLastValues();

  if (clearBody) {
    tft.fillRect(0, 53, SCREEN_W, SCREEN_H - 53, COL_BG);
  }

  switch (action) {
    case ACTION_OSC_ENABLED:
    case ACTION_OSC_LEVEL:
    case ACTION_OSC_OCTAVE:
    case ACTION_OSC_PHASE:
    case ACTION_OSC_PULSE_WIDTH:
    case ACTION_OSC_PAN:
    case ACTION_OSC_KEY_TRACK:
    case ACTION_OSC_MOD_AMOUNT:
      drawOscillatorParameterEdit(accent);
      break;

    case ACTION_VOICE_WAVE:
      drawVoiceWaveEdit(accent);
      break;

    case ACTION_VOICE_TUNE:
      drawVoiceTuneEdit(accent);
      break;

    case ACTION_VOICE_FOLD:
      drawVoiceFoldEdit(accent);
      break;

    case ACTION_VOICE_DETUNE:
      drawVoiceDetuneEdit(accent);
      break;

    case ACTION_VOICE_SCOPE:
      drawVoiceScopeEdit(accent);
      break;

    case ACTION_PITCH:
      drawPitchEdit(accent);
      break;

    case ACTION_LEVEL:
      drawLevelEdit(accent);
      break;

    case ACTION_FILTER_POWER:
      drawFilterPowerEdit(accent);
      break;

    case ACTION_FILTER_TYPE:
      drawFilterTypeEdit(accent);
      break;

    case ACTION_FILTER_CUTOFF:
      drawFilterCutoffEdit(accent);
      break;

    case ACTION_ADSR:
      drawAdsrEdit(accent);
      break;

    case ACTION_ATTACK:
      drawAttackEdit(accent);
      break;

    case ACTION_DECAY:
      drawDecayEdit(accent);
      break;

    case ACTION_SUSTAIN:
      drawSustainEdit(accent);
      break;

    case ACTION_RELEASE:
      drawReleaseEdit(accent);
      break;

    case ACTION_MOD_ROUTE_ENABLED:
    case ACTION_MOD_ROUTE_AMOUNT:
      drawModulationRouteEdit(accent);
      break;

    case ACTION_SCOPE:
      drawScopeEdit(accent);
      break;

    case ACTION_AUDIO_TEST:
      drawAudioTestEdit(accent);
      break;

    case ACTION_SEQ_START:
      drawSequencerStartEdit(accent);
      break;

    case ACTION_SEQ_STOP:
      drawSequencerStopEdit(accent);
      break;

    case ACTION_SYNTH_VIEW:
      drawSynthView(accent, clearBody, false);
      break;

    case ACTION_SEQ_BPM:
      drawSequencerBpmEdit(accent);
      break;

    case ACTION_SEQ_VOICE:
      drawSequencerVoiceEdit(accent);
      break;

    case ACTION_SEQ_STEP:
      drawSequencerStepEdit(accent);
      break;

    case ACTION_SEQ_NOTE:
      drawSequencerNoteEdit(accent);
      break;

    case ACTION_SEQ_LENGTH:
      drawSequencerLengthEdit(accent);
      break;

    case ACTION_SEQ_PAGE:
      drawSequencerPageEdit(accent);
      break;

    case ACTION_SEQ_AUTO_SCROLL:
      drawSequencerAutoScrollEdit(accent);
      break;

    case ACTION_SEQ_STEP_ENABLE:
      drawSequencerStepEnableEdit(accent);
      break;

    case ACTION_MIX_VOICE_1:
    case ACTION_MIX_VOICE_2:
      drawMixerWaveEdit(getMixerWaveIndex(action), accent);
      break;

    case ACTION_MIX_KICK:
      drawMixerKickEdit(accent);
      break;

    case ACTION_KICK_PITCH:
      drawKickPitchEdit(accent);
      break;

    case ACTION_KICK_DECAY:
      drawKickDecayEdit(accent);
      break;

    case ACTION_KICK_SWEEP:
      drawKickSweepEdit(accent);
      break;

    case ACTION_KICK_ATTACK:
      drawKickAttackEdit(accent);
      break;

    case ACTION_KICK_DRIVE:
      drawKickDriveEdit(accent);
      break;

    case ACTION_DELAY_MIX:
    case ACTION_DELAY_TIME:
    case ACTION_DELAY_FEEDBACK:
    case ACTION_REVERB_MIX:
    case ACTION_REVERB_ROOM:
      drawEffectEdit(action, accent);
      break;

    case ACTION_PRESET_SLOT:
    case ACTION_PRESET_LOAD:
    case ACTION_PRESET_SAVE:
      drawPresetEdit(action, accent);
      break;

    case ACTION_INPUTS:
      drawInputsEdit(accent);
      break;

    case ACTION_DISPLAY_TEST:
      drawDisplayTestEdit(accent);
      break;

    case ACTION_SURFACE_MODE:
      drawControlSurfaceEdit(accent);
      break;

    case ACTION_OLED_DEBUG:
      drawOledDebugEdit(accent);
      break;

    case ACTION_MCP_DEBUG:
      drawMcpDebugEdit(accent);
      break;

    case ACTION_NEOKEY_DEBUG:
      drawNeoKeyDebugEdit(accent);
      break;

    case ACTION_SERIAL_DEBUG:
      drawSerialDebugEdit(accent);
      break;

    default:
      break;
  }
}

void updateEditFast() {
  if (getSelectedAction() == ACTION_CONNECTION_STATUS) {
    static uint32_t lastConnectionDrawMs = 0;
    static uint32_t lastConnectionHash = 0xFFFFFFFFUL;
    uint32_t now = millis();
    uint32_t statusHash = getConnectionStatusHash();

    if (fullRedraw || lastDrawnMode != MODE_EDIT || valueDirty ||
        statusHash != lastConnectionHash || now - lastConnectionDrawMs >= 500) {
      drawConnectionStatusScreen(false, 0);
      lastConnectionDrawMs = now;
      lastConnectionHash = statusHash;
      valueDirty = false;
      fullRedraw = false;
      lastDrawnMode = MODE_EDIT;
    }
    return;
  }

  if (isPatternEditor()) {
    bool clearView = fullRedraw || lastDrawnMode != MODE_EDIT;
    if (!clearView && !valueDirty && !editValueChanged()) return;
    bool detailsChanged = clearView || valueDirty ||
      activeSequencerTrack != lastSequencerTrack ||
      sequencerSelectedStep != lastSequencerSelectedStep ||
      getSequencerNoteChecksum() != lastSequencerNoteChecksum;
    drawPatternEditor(clearView, detailsChanged);
    valueDirty = false;
    fullRedraw = false;
    lastDrawnMode = MODE_EDIT;
    saveLastValues();
    return;
  }

  if (getSelectedAction() == ACTION_SYNTH_VIEW) {
    bool clearView = fullRedraw || lastDrawnMode != MODE_EDIT;
    bool explicitViewUpdate = valueDirty;

    if (!clearView && !explicitViewUpdate && !editValueChanged()) {
      return;
    }

    int itemIndex = getMenuItemIndex(selectedItem);
    uint16_t accent = getAccentColor(itemIndex);
    bool staticViewChanged =
      (int)transportRunning != lastTransportRunning ||
      (int)sequencerEnabled != lastSequencerEnabled ||
      sequencerBpm != lastSequencerBpm ||
      sequencerLength != lastSequencerLength ||
      activeSequencerTrack != lastSequencerTrack ||
      sequencerSelectedStep != lastSequencerSelectedStep ||
      getSequencerPage() != lastSequencerPage ||
      (DaisyBridge::isCursorSynchronized() ? 1 : 0) != lastDaisyCursorSynchronized ||
      (DaisyBridge::isConnected() ? 1 : 0) != lastDaisyConnected ||
      (int)sequencerAutoScroll != lastSequencerAutoScroll ||
      getSequencerNoteChecksum() != lastSequencerNoteChecksum ||
      getWaveVoiceParamChecksum() != lastWaveVoiceParamChecksum ||
      getLevelChecksum(waveVoiceLevels, WAVE_VOICE_COUNT) != lastWaveVoiceChecksum ||
      kickLevel != lastKickLevel;
    bool playheadOnly = !clearView && !explicitViewUpdate && !staticViewChanged &&
                        sequencerCurrentStep != lastSequencerCurrentStep;

    valueDirty = false;
    fullRedraw = false;
    lastDrawnMode = MODE_EDIT;
    saveLastValues();
    drawSynthView(accent, clearView, playheadOnly);
    return;
  }

  if (fullRedraw || lastDrawnMode != MODE_EDIT) {
    drawEditBase();

    fullRedraw = false;
    lastDrawnMode = MODE_EDIT;
  }

  drawEditValue();
}

// =======================
// ABOUT DRAW
// =======================

void drawAboutScreen() {
  drawStageBackdrop();

  drawHeader("ABOUT");

  tft.setTextColor(COL_GOLD);
  tft.setTextSize(2);
  tft.setCursor(8, 30);
  tft.print("POLYGLIDER");

  tft.setTextColor(COL_NOTE);
  tft.setTextSize(1);
  tft.setCursor(10, 53);
  tft.print("LCD SYNTH PANEL");

  tft.setTextColor(COL_INK);
  tft.setCursor(10, 68);
  tft.print("ESP32 + ST7735S");

  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 81);
  tft.print("encoder + K0 control");

  drawMiniKeyboard(10, 93);
  drawWavePreview(86, 94, 62, 18, COL_NOTE);

  lastDrawnMode = MODE_ABOUT;
  fullRedraw = false;
}

// =======================
// DRAW DISPATCH
// =======================

void drawUi() {
  if (uiMode == MODE_BOOT) {
    drawBootScreen();
    return;
  }

  if (uiMode == MODE_PREMENU) {
    updatePreMenuFast();
  }

  else if (uiMode == MODE_MENU) {
    updateMenuFast();
  }

  else if (uiMode == MODE_EDIT) {
    updateEditFast();
  }

  else if (uiMode == MODE_ABOUT) {
    if (fullRedraw || lastDrawnMode != MODE_ABOUT) {
      drawAboutScreen();
    }
  }

}
