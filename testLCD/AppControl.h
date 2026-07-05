#pragma once

// =======================
// UTILITY
// =======================

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

int wrapIndex(int value, int count) {
  while (value < 0) value += count;
  while (value >= count) value -= count;
  return value;
}

const int DISCRETE_EDIT_STEP_THRESHOLD = 2;
int discreteEditAccumulator = 0;
int lastDiscreteEditAction = -1;

bool isDiscreteEditAction(MenuAction action) {
  switch (action) {
    case ACTION_VOICE_WAVE:
    case ACTION_VOICE_TUNE:
    case ACTION_FILTER_POWER:
    case ACTION_FILTER_TYPE:
    case ACTION_FFT_RANGE:
    case ACTION_FFT_WINDOW:
    case ACTION_FFT_HOLD:
    case ACTION_FFT_BINS:
    case ACTION_BT_POWER:
    case ACTION_AUDIO_TEST:
    case ACTION_SEQ_START:
    case ACTION_SEQ_STOP:
    case ACTION_SEQ_POWER:
    case ACTION_SYNTH_VIEW:
    case ACTION_SEQ_STEP:
    case ACTION_SEQ_DIRECTION:
    case ACTION_SEQ_STEP_ENABLE:
    case ACTION_SEQ_RANDOMIZE:
    case ACTION_SEQ_CLEAR:
    case ACTION_SEQ_ROTATE:
    case ACTION_PRESET_SLOT:
      return true;

    default:
      return false;
  }
}

int getEditDeltaForAction(MenuAction action, int delta) {
  if (!isDiscreteEditAction(action)) {
    discreteEditAccumulator = 0;
    lastDiscreteEditAction = (int)action;
    return delta;
  }

  if (lastDiscreteEditAction != (int)action) {
    discreteEditAccumulator = 0;
    lastDiscreteEditAction = (int)action;
  }

  discreteEditAccumulator += delta;

  if (discreteEditAccumulator >= DISCRETE_EDIT_STEP_THRESHOLD) {
    discreteEditAccumulator -= DISCRETE_EDIT_STEP_THRESHOLD;
    return 1;
  }

  if (discreteEditAccumulator <= -DISCRETE_EDIT_STEP_THRESHOLD) {
    discreteEditAccumulator += DISCRETE_EDIT_STEP_THRESHOLD;
    return -1;
  }

  return 0;
}

void btAudioDataReceived() {
  btLastAudioMs = millis();
  btPacketCount++;
}

void captureBluetoothFftSamples(const uint8_t* data, uint32_t length) {
  const int16_t* pcm = (const int16_t*)data;
  uint32_t frames = length / 4;

  for (uint32_t i = 0; i < frames; i++) {
    btFftDecimator++;

    if (btFftDecimator < BT_FFT_DECIMATE) {
      continue;
    }

    btFftDecimator = 0;

    int32_t left = pcm[i * 2];
    int32_t right = pcm[i * 2 + 1];
    int16_t mono = (int16_t)((left + right) / 2);

    btFftSamples[btFftWriteIndex] = mono;
    btFftWriteIndex = (btFftWriteIndex + 1) % BT_FFT_SAMPLES;

    if (btFftSampleCount < BT_FFT_SAMPLES) {
      btFftSampleCount++;
    }
  }
}

void btAudioStreamReceived(const uint8_t* data, uint32_t length) {
  captureBluetoothFftSamples(data, length);
  btAudioDataReceived();
}

void btRssiReceived(esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi) {
  btRssiDelta = rssi.rssi_delta;
  btRssiValid = true;
}

void applyBluetoothStartVolume(bool force) {
  if (!bluetoothEnabled || !btAudioStarted) return;

  bool connected = a2dpSink.is_connected();
  uint32_t now = millis();

  if (!connected) {
    lastBtConnected = false;
    btStartVolumeApplied = false;
    btStartVolumeApplyCount = 0;
    return;
  }

  // Quando Linux si collega puo partire con un volume ricordato molto basso.
  // Lo forziamo alla connessione e una sola seconda volta poco dopo,
  // cosi poi non combattiamo il volume scelto dall'utente.
  bool justConnected = !lastBtConnected;
  bool retryDue =
    btStartVolumeApplyCount == 1 &&
    now - lastBtStartVolumeMs >= BT_START_VOLUME_RETRY_MS;

  if (force || justConnected || !btStartVolumeApplied || retryDue) {
    a2dpSink.set_volume(BT_START_VOLUME);
    btStartVolumeApplied = true;
    lastBtStartVolumeMs = now;

    if (btStartVolumeApplyCount < 2) {
      btStartVolumeApplyCount++;
    }
  }

  lastBtConnected = true;
}

void startDacI2s() {
  if (dacI2sStarted) return;

  auto cfg = btI2s.defaultConfig();
  cfg.pin_bck = PCM5102_BCK;
  cfg.pin_ws = PCM5102_LRCK;
  cfg.pin_data = PCM5102_DIN;
  btI2s.begin(cfg);

  dacI2sStarted = true;
}

void startBluetoothAudio() {
  if (!bluetoothEnabled || btAudioStarted) return;

  startDacI2s();

  a2dpSink.set_stream_reader(btAudioStreamReceived, true);
  a2dpSink.set_on_data_received(btAudioDataReceived);
  a2dpSink.set_rssi_active(true);
  a2dpSink.set_rssi_callback(btRssiReceived);
  a2dpSink.start(BT_AUDIO_NAME);

  btAudioStarted = true;
  btStartedAtMs = millis();

  // Default locale del sink; verra ribadito anche appena il PC si collega.
  a2dpSink.set_volume(BT_START_VOLUME);
}

void stopBluetoothAudio() {
  if (!btAudioStarted) return;

  a2dpSink.end();

  btAudioStarted = false;
  btLastAudioMs = 0;
  lastBtConnected = false;
  btStartVolumeApplied = false;
  btStartVolumeApplyCount = 0;
  btRssiValid = false;
}

bool isBluetoothStreaming() {
  if (!bluetoothEnabled || !btAudioStarted) return false;
  if (a2dpSink.is_output_active()) return true;
  if (btLastAudioMs == 0) return false;

  return millis() - btLastAudioMs < BT_STREAM_TIMEOUT_MS;
}

const char* getBluetoothStatusText() {
  if (!bluetoothEnabled) return "OFF";
  if (!btAudioStarted) return "OFF";
  if (isBluetoothStreaming()) return "PLAY";
  if (a2dpSink.is_connected()) return "CONN";
  return "PAIR";
}

void updateBluetoothRssi() {
  if (!bluetoothEnabled || !btAudioStarted || !a2dpSink.is_connected()) {
    btRssiValid = false;
    return;
  }

  uint32_t now = millis();

  if (now - lastBtRssiRequestMs >= BT_RSSI_REFRESH_MS) {
    a2dpSink.update_rssi();
    lastBtRssiRequestMs = now;
  }
}

int getBluetoothSignalBars() {
  if (!bluetoothEnabled || !btAudioStarted || !a2dpSink.is_connected()) return 0;

  if (!btRssiValid) {
    return isBluetoothStreaming() ? 3 : 2;
  }

  int delta = abs((int)btRssiDelta);

  if (delta <= 5) return 4;
  if (delta <= 15) return 3;
  if (delta <= 30) return 2;
  return 1;
}

bool isWaveVoicePage(MenuPage page) {
  return page == MENU_PAGE_WAVE_SINE ||
         page == MENU_PAGE_WAVE_SQUARE ||
         page == MENU_PAGE_WAVE_TRIANGLE ||
         page == MENU_PAGE_WAVE_SAW;
}

int getWaveVoiceIndexForPage(MenuPage page) {
  switch (page) {
    case MENU_PAGE_WAVE_SINE:
      return 0;

    case MENU_PAGE_WAVE_SQUARE:
      return 1;

    case MENU_PAGE_WAVE_TRIANGLE:
      return 2;

    case MENU_PAGE_WAVE_SAW:
      return 3;

    default:
      return 0;
  }
}

int getCurrentWaveVoiceIndex() {
  if (isWaveVoicePage(currentMenuPage)) {
    activeWaveVoiceIndex = getWaveVoiceIndexForPage(currentMenuPage);
  }

  activeWaveVoiceIndex = clampInt(activeWaveVoiceIndex, 0, WAVE_VOICE_COUNT - 1);
  return activeWaveVoiceIndex;
}

bool isMenuSubmenuAction(MenuAction action) {
  return action == ACTION_WAVE_SINE_MENU ||
         action == ACTION_WAVE_SQUARE_MENU ||
         action == ACTION_WAVE_TRIANGLE_MENU ||
         action == ACTION_WAVE_SAW_MENU ||
         action == ACTION_FILTER_MENU ||
         action == ACTION_KICK_MENU ||
         action == ACTION_TRANSPORT_MENU;
}

bool isImmediateMenuAction(MenuAction action) {
  return action == ACTION_SEQ_START ||
         action == ACTION_SEQ_STOP ||
         action == ACTION_PRESET_LOAD ||
         action == ACTION_PRESET_SAVE;
}

void startTransport() {
  transportRunning = true;
  sequencerEnabled = true;
  resetSequencerClock();
  synthRetriggerRequested = true;

  if (kickLevel > 0) {
    triggerKickVoice();
  }
}

void stopTransport() {
  transportRunning = false;
  sequencerEnabled = false;
  kickVoiceActive = false;
  sequencerSampleCounter = 0;
  synthEnvelopeSample = 0;
  synthEnvelopeSmoothed = 0.0f;
}

void executeImmediateMenuAction(MenuAction action) {
  if (action == ACTION_SEQ_START) {
    startTransport();
  } else if (action == ACTION_SEQ_STOP) {
    stopTransport();
  } else if (action == ACTION_PRESET_LOAD) {
    presetOperationOk = loadCurrentPreset();
    presetOperationText = presetOperationOk ? "LOADED" : "EMPTY";
  } else if (action == ACTION_PRESET_SAVE) {
    presetOperationOk = saveCurrentPreset();
    presetOperationText = presetOperationOk ? "SAVED" : "ERROR";
  }

  valueDirty = true;
  fullRedraw = true;
}

MenuPage getSubmenuPageForAction(MenuAction action) {
  switch (action) {
    case ACTION_WAVE_SINE_MENU:
      return MENU_PAGE_WAVE_SINE;

    case ACTION_WAVE_SQUARE_MENU:
      return MENU_PAGE_WAVE_SQUARE;

    case ACTION_WAVE_TRIANGLE_MENU:
      return MENU_PAGE_WAVE_TRIANGLE;

    case ACTION_WAVE_SAW_MENU:
      return MENU_PAGE_WAVE_SAW;

    case ACTION_FILTER_MENU:
      return MENU_PAGE_FILTER;

    case ACTION_KICK_MENU:
      return MENU_PAGE_KICK;

    case ACTION_TRANSPORT_MENU:
      return MENU_PAGE_TRANSPORT;

    default:
      return MENU_PAGE_ROOT;
  }
}

bool isInSubmenu() {
  return currentMenuPage != MENU_PAGE_ROOT;
}

void resetMenuSelection() {
  selectedItem = 0;
  topItem = 0;
  lastSelectedItem = -1;
  lastTopItem = -1;
}

void enterSubmenu(MenuAction action) {
  MenuPage page = getSubmenuPageForAction(action);
  if (page == MENU_PAGE_ROOT) return;

  currentMenuPage = page;

  if (isWaveVoicePage(page)) {
    activeWaveVoiceIndex = getWaveVoiceIndexForPage(page);
  }

  resetMenuSelection();
  fullRedraw = true;
  valueDirty = true;
}

void leaveSubmenu() {
  currentMenuPage = MENU_PAGE_ROOT;
  resetMenuSelection();
  fullRedraw = true;
  valueDirty = true;
}

const uint8_t* getCurrentMenuItems(int* count) {
  if (currentMenuPage == MENU_PAGE_FILTER) {
    *count = sizeof(MENU_FILTER_ITEMS) / sizeof(MENU_FILTER_ITEMS[0]);
    return MENU_FILTER_ITEMS;
  }

  if (currentMenuPage == MENU_PAGE_KICK) {
    *count = sizeof(MENU_KICK_ITEMS) / sizeof(MENU_KICK_ITEMS[0]);
    return MENU_KICK_ITEMS;
  }

  if (currentMenuPage == MENU_PAGE_TRANSPORT) {
    *count = sizeof(MENU_TRANSPORT_ITEMS) / sizeof(MENU_TRANSPORT_ITEMS[0]);
    return MENU_TRANSPORT_ITEMS;
  }

  if (isWaveVoicePage(currentMenuPage)) {
    *count = sizeof(MENU_WAVE_VOICE_ITEMS) / sizeof(MENU_WAVE_VOICE_ITEMS[0]);
    return MENU_WAVE_VOICE_ITEMS;
  }

  *count = areaChoices[activeType].itemCount;
  return areaChoices[activeType].items;
}

const char* getCurrentMenuTitle() {
  static char title[24];
  const char* source = areaChoices[activeType].label;

  if (isWaveVoicePage(currentMenuPage)) {
    snprintf(title, sizeof(title), "VOICE %d", getCurrentWaveVoiceIndex() + 1);
    return title;
  }

  if (currentMenuPage == MENU_PAGE_FILTER) {
    source = "FILTER";
  }
  else if (currentMenuPage == MENU_PAGE_KICK) {
    source = "KICK DESIGN";
  }
  else if (currentMenuPage == MENU_PAGE_TRANSPORT) {
    source = "TRANSPORT";
  }

  size_t i = 0;
  for (; source[i] != '\0' && i < sizeof(title) - 1; i++) {
    char c = source[i];
    title[i] = c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
  }
  title[i] = '\0';
  return title;
}

int getActiveMenuCount() {
  int count = 0;
  getCurrentMenuItems(&count);
  return count;
}

int getMenuItemIndex(int visibleIndex) {
  int count = 0;
  const uint8_t* items = getCurrentMenuItems(&count);

  if (visibleIndex < 0 || visibleIndex >= count) {
    return 0;
  }

  return items[visibleIndex];
}

MenuAction getSelectedAction() {
  int itemIndex = getMenuItemIndex(selectedItem);
  return menuItems[itemIndex].action;
}

bool isSynthAction(MenuAction action) {
  return action == ACTION_WAVE_SINE_MENU ||
         action == ACTION_WAVE_SQUARE_MENU ||
         action == ACTION_WAVE_TRIANGLE_MENU ||
         action == ACTION_WAVE_SAW_MENU ||
         action == ACTION_FILTER_MENU ||
         action == ACTION_KICK_MENU ||
         action == ACTION_TRANSPORT_MENU ||
         action == ACTION_VOICE_WAVE ||
         action == ACTION_VOICE_TUNE ||
         action == ACTION_VOICE_FOLD ||
         action == ACTION_VOICE_PHASE ||
         action == ACTION_VOICE_SCOPE ||
         action == ACTION_PITCH ||
         action == ACTION_LEVEL ||
         action == ACTION_FILTER_POWER ||
         action == ACTION_FILTER_TYPE ||
         action == ACTION_FILTER_CUTOFF ||
         action == ACTION_ADSR ||
         action == ACTION_ATTACK ||
         action == ACTION_DECAY ||
         action == ACTION_SUSTAIN ||
         action == ACTION_RELEASE ||
         action == ACTION_AUDIO_TEST ||
         action == ACTION_SEQ_START ||
         action == ACTION_SEQ_STOP ||
         action == ACTION_SEQ_POWER ||
         action == ACTION_SYNTH_VIEW ||
         action == ACTION_SEQ_BPM ||
         action == ACTION_SEQ_STEP ||
         action == ACTION_SEQ_NOTE ||
         action == ACTION_SEQ_LENGTH ||
         action == ACTION_SEQ_DIRECTION ||
         action == ACTION_SEQ_GATE ||
         action == ACTION_SEQ_TRANSPOSE ||
         action == ACTION_SEQ_STEP_ENABLE ||
         action == ACTION_SEQ_RANDOMIZE ||
         action == ACTION_SEQ_CLEAR ||
         action == ACTION_SEQ_ROTATE ||
         action == ACTION_MIX_SINE ||
         action == ACTION_MIX_SQUARE ||
         action == ACTION_MIX_TRIANGLE ||
         action == ACTION_MIX_SAW ||
         action == ACTION_MIX_KICK ||
         action == ACTION_KICK_PITCH ||
         action == ACTION_KICK_DECAY ||
         action == ACTION_KICK_SWEEP ||
         action == ACTION_KICK_CLICK ||
         action == ACTION_KICK_DRIVE ||
         action == ACTION_DELAY_MIX ||
         action == ACTION_DELAY_TIME ||
         action == ACTION_DELAY_FEEDBACK ||
         action == ACTION_REVERB_MIX ||
         action == ACTION_REVERB_ROOM;
}

void wrapSelectedMenu() {
  int count = getActiveMenuCount();
  if (count <= 0) return;

  if (count <= VISIBLE_ROWS) {
    selectedItem = wrapIndex(selectedItem, count);
    topItem = 0;
    return;
  }

  if (selectedItem < 0) {
    selectedItem = count - 1;
    topItem = selectedItem;
    return;
  }

  if (selectedItem >= count) {
    selectedItem = 0;
  }

  int position = wrapIndex(selectedItem - topItem, count);
  if (position >= VISIBLE_ROWS) {
    int forwardEdge = wrapIndex(topItem + VISIBLE_ROWS, count);
    topItem = selectedItem == forwardEdge ? wrapIndex(topItem + 1, count) : selectedItem;
  }
}

void wrapSelectedType() {
  if (selectedType < 0) {
    selectedType = TYPE_COUNT - 1;
    topType = selectedType;
    return;
  }

  if (selectedType >= TYPE_COUNT) {
    selectedType = 0;
  }

  int position = wrapIndex(selectedType - topType, TYPE_COUNT);
  if (position >= VISIBLE_TYPE_ROWS) {
    int forwardEdge = wrapIndex(topType + VISIBLE_TYPE_ROWS, TYPE_COUNT);
    topType = selectedType == forwardEdge ? wrapIndex(topType + 1, TYPE_COUNT) : selectedType;
  }
}

int32_t getMenuNavigationStep(int32_t steps) {
  if (steps == 0) return 0;

  uint32_t now = millis();

  if (lastMenuMoveMs != 0 && now - lastMenuMoveMs < menuStepCooldownMs) {
    return 0;
  }

  lastMenuMoveMs = now;

  return steps > 0 ? 1 : -1;
}

void applyTypeChoice(int typeIndex) {
  typeIndex = clampInt(typeIndex, 0, TYPE_COUNT - 1);

  activeType = typeIndex;
  selectedType = typeIndex;
  currentMenuPage = MENU_PAGE_ROOT;
  activeWaveVoiceIndex = 0;

  resetMenuSelection();
  wrapSelectedMenu();

  lastWaveform = -1;
  lastFrequency = -1;
  lastVolume = -1;
  lastAttack = -1;
  lastDecay = -1;
  lastSustain = -1;
  lastReleaseMs = -1;
  lastWaveFold = -1;
  lastWavePhase = -1;
  lastWaveVoiceParamChecksum = -1;
  lastKickParamChecksum = -1;
  lastFftRangeIndex = -1;
  lastFftWindow = -1;
  lastFftSmoothing = -1;
  lastFftBinsIndex = -1;
  lastFftPeakHold = -1;
}

const char* getValueText(int itemIndex) {
  static char buffer[24];
  MenuAction action = menuItems[itemIndex].action;

  switch (action) {
    case ACTION_WAVE_SINE_MENU:
    case ACTION_WAVE_SQUARE_MENU:
    case ACTION_WAVE_TRIANGLE_MENU:
    case ACTION_WAVE_SAW_MENU:
    case ACTION_FILTER_MENU:
    case ACTION_KICK_MENU:
    case ACTION_TRANSPORT_MENU:
      snprintf(buffer, sizeof(buffer), "OPEN");
      break;

    case ACTION_VOICE_WAVE:
      snprintf(buffer, sizeof(buffer), "%s", waveformNames[waveVoiceShape[getCurrentWaveVoiceIndex()]]);
      break;

    case ACTION_VOICE_TUNE:
      snprintf(buffer, sizeof(buffer), "%+dst", waveVoiceTune[getCurrentWaveVoiceIndex()]);
      break;

    case ACTION_VOICE_FOLD:
      snprintf(buffer, sizeof(buffer), "%d%%", waveVoiceFold[getCurrentWaveVoiceIndex()]);
      break;

    case ACTION_VOICE_PHASE:
      snprintf(buffer, sizeof(buffer), "%ddeg", waveVoicePhase[getCurrentWaveVoiceIndex()]);
      break;

    case ACTION_VOICE_SCOPE:
      snprintf(buffer, sizeof(buffer), "Live");
      break;

    case ACTION_PITCH:
      snprintf(buffer, sizeof(buffer), "%dHz", frequency);
      break;

    case ACTION_LEVEL:
      snprintf(buffer, sizeof(buffer), "%d%%", volume);
      break;

    case ACTION_FILTER_POWER:
      snprintf(buffer, sizeof(buffer), "%s", filterEnabled ? "ON" : "OFF");
      break;

    case ACTION_FILTER_TYPE:
      snprintf(buffer, sizeof(buffer), "%s", FILTER_TYPE_NAMES[filterType]);
      break;

    case ACTION_FILTER_CUTOFF:
      snprintf(buffer, sizeof(buffer), "%dHz", filterCutoff);
      break;

    case ACTION_ADSR:
      snprintf(buffer, sizeof(buffer), "VIEW");
      break;

    case ACTION_ATTACK:
      snprintf(buffer, sizeof(buffer), "%dms", attack);
      break;

    case ACTION_DECAY:
      snprintf(buffer, sizeof(buffer), "%dms", decay);
      break;

    case ACTION_SUSTAIN:
      snprintf(buffer, sizeof(buffer), "S%d%%", sustain);
      break;

    case ACTION_RELEASE:
      snprintf(buffer, sizeof(buffer), "%dms", releaseMs);
      break;

    case ACTION_SPECTRUM:
      snprintf(buffer, sizeof(buffer), "Live");
      break;

    case ACTION_BT_FFT:
      snprintf(buffer, sizeof(buffer), "%s", getBluetoothStatusText());
      break;

    case ACTION_FFT_RANGE:
      snprintf(buffer, sizeof(buffer), "%s", FFT_RANGE_LABELS[fftRangeIndex]);
      break;

    case ACTION_FFT_WINDOW:
      snprintf(buffer, sizeof(buffer), "%s", FFT_WINDOW_NAMES[fftWindow]);
      break;

    case ACTION_FFT_SMOOTH:
      snprintf(buffer, sizeof(buffer), "%d%%", fftSmoothing);
      break;

    case ACTION_FFT_HOLD:
      snprintf(buffer, sizeof(buffer), "%s", fftPeakHold ? "ON" : "OFF");
      break;

    case ACTION_FFT_BINS:
      snprintf(buffer, sizeof(buffer), "%d", FFT_BIN_VALUES[fftBinsIndex]);
      break;

    case ACTION_SCOPE:
      snprintf(buffer, sizeof(buffer), "Live");
      break;

    case ACTION_BT_RECEIVER:
      snprintf(buffer, sizeof(buffer), "%s", getBluetoothStatusText());
      break;

    case ACTION_BT_DEVICE:
      snprintf(buffer, sizeof(buffer), "NAME");
      break;

    case ACTION_BT_I2S:
      snprintf(buffer, sizeof(buffer), "%d/%d/%d", PCM5102_BCK, PCM5102_LRCK, PCM5102_DIN);
      break;

    case ACTION_BT_OUTPUT:
      snprintf(buffer, sizeof(buffer), "I2S");
      break;

    case ACTION_BT_POWER:
      snprintf(buffer, sizeof(buffer), "%s", bluetoothEnabled ? "ON" : "OFF");
      break;

    case ACTION_AUDIO_TEST:
      snprintf(buffer, sizeof(buffer), "%s", waveformNames[waveform]);
      break;

    case ACTION_SEQ_START:
      snprintf(buffer, sizeof(buffer), "%s", transportRunning && sequencerEnabled ? "RUN" : "GO");
      break;

    case ACTION_SEQ_STOP:
      snprintf(buffer, sizeof(buffer), "%s", transportRunning ? "HALT" : "OFF");
      break;

    case ACTION_SEQ_POWER:
      snprintf(buffer, sizeof(buffer), "%s", sequencerEnabled ? "ON" : "OFF");
      break;

    case ACTION_SYNTH_VIEW:
      snprintf(buffer, sizeof(buffer), "VIEW");
      break;

    case ACTION_SEQ_BPM:
      snprintf(buffer, sizeof(buffer), "%dBPM", sequencerBpm);
      break;

    case ACTION_SEQ_STEP:
      snprintf(buffer, sizeof(buffer), "S%d", sequencerSelectedStep + 1);
      break;

    case ACTION_SEQ_NOTE:
      snprintf(buffer, sizeof(buffer), "%+dst", sequencerStepNotes[sequencerSelectedStep]);
      break;

    case ACTION_SEQ_LENGTH:
      snprintf(buffer, sizeof(buffer), "%d", sequencerLength);
      break;

    case ACTION_SEQ_DIRECTION:
      snprintf(buffer, sizeof(buffer), "%s", SEQUENCER_DIRECTION_NAMES[sequencerDirection]);
      break;

    case ACTION_SEQ_GATE:
      snprintf(buffer, sizeof(buffer), "%d%%", sequencerGate);
      break;

    case ACTION_SEQ_TRANSPOSE:
      snprintf(buffer, sizeof(buffer), "%+dst", sequencerTranspose);
      break;

    case ACTION_SEQ_STEP_ENABLE:
      snprintf(buffer, sizeof(buffer), "%s", sequencerStepEnabled[sequencerSelectedStep] ? "ON" : "REST");
      break;

    case ACTION_SEQ_RANDOMIZE:
      snprintf(buffer, sizeof(buffer), "GO");
      break;

    case ACTION_SEQ_CLEAR:
      snprintf(buffer, sizeof(buffer), "GO");
      break;

    case ACTION_SEQ_ROTATE:
      snprintf(buffer, sizeof(buffer), "GO");
      break;

    case ACTION_MIX_SINE:
    case ACTION_MIX_SQUARE:
    case ACTION_MIX_TRIANGLE:
    case ACTION_MIX_SAW:
    {
      int waveIndex = getMixerWaveIndex(action);
      snprintf(buffer, sizeof(buffer), "%d%%", waveVoiceLevels[waveIndex]);
      break;
    }

    case ACTION_MIX_KICK:
      snprintf(buffer, sizeof(buffer), "%d%%", kickLevel);
      break;

    case ACTION_KICK_PITCH:
      snprintf(buffer, sizeof(buffer), "%dHz", kickPitch);
      break;

    case ACTION_KICK_DECAY:
      snprintf(buffer, sizeof(buffer), "%dms", kickDecay);
      break;

    case ACTION_KICK_SWEEP:
      snprintf(buffer, sizeof(buffer), "%dHz", kickSweep);
      break;

    case ACTION_KICK_CLICK:
      snprintf(buffer, sizeof(buffer), "%d%%", kickClick);
      break;

    case ACTION_KICK_DRIVE:
      snprintf(buffer, sizeof(buffer), "%d%%", kickDrive);
      break;

    case ACTION_DELAY_MIX:
      snprintf(buffer, sizeof(buffer), "%d%%", delayMix);
      break;

    case ACTION_DELAY_TIME:
      snprintf(buffer, sizeof(buffer), "%dms", delayTimeMs);
      break;

    case ACTION_DELAY_FEEDBACK:
      snprintf(buffer, sizeof(buffer), "%d%%", delayFeedback);
      break;

    case ACTION_REVERB_MIX:
      snprintf(buffer, sizeof(buffer), "%d%%", reverbMix);
      break;

    case ACTION_REVERB_ROOM:
      snprintf(buffer, sizeof(buffer), "%d%%", reverbRoom);
      break;

    case ACTION_PRESET_SLOT:
      snprintf(buffer, sizeof(buffer), "P%d", presetSlot);
      break;

    case ACTION_PRESET_LOAD:
    case ACTION_PRESET_SAVE:
      snprintf(buffer, sizeof(buffer), "P%d", presetSlot);
      break;

    case ACTION_INPUTS:
      snprintf(buffer, sizeof(buffer), "IO");
      break;

    case ACTION_DISPLAY_TEST:
      snprintf(buffer, sizeof(buffer), "LCD");
      break;

    case ACTION_ABOUT:
      snprintf(buffer, sizeof(buffer), "ABOUT");
      break;

    default:
      snprintf(buffer, sizeof(buffer), "-");
      break;
  }

  return buffer;
}

int getSequencerLength() {
  return clampInt(sequencerLength, 1, SEQUENCER_STEP_COUNT);
}

void clampSequencerIndexes() {
  int length = getSequencerLength();

  if (sequencerSelectedStep >= length) {
    sequencerSelectedStep = length - 1;
  }

  if (sequencerCurrentStep >= length) {
    sequencerCurrentStep = 0;
  }

  if (sequencerSelectedStep < 0) sequencerSelectedStep = 0;
  if (sequencerCurrentStep < 0) sequencerCurrentStep = 0;
}

void randomizeSequencerPattern() {
  int length = getSequencerLength();

  for (int i = 0; i < length; i++) {
    sequencerStepNotes[i] = random(SEQUENCER_MIN_NOTE / 2, SEQUENCER_MAX_NOTE / 2 + 1);
    sequencerStepEnabled[i] = random(0, 100) >= 18;
  }

  sequencerStepEnabled[0] = true;
  resetSequencerClock();
}

void clearSequencerPattern() {
  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    sequencerStepNotes[i] = 0;
    sequencerStepEnabled[i] = true;
  }

  resetSequencerClock();
}

void rotateSequencerPattern(int direction) {
  int length = getSequencerLength();
  if (length <= 1) return;

  if (direction >= 0) {
    int lastNote = sequencerStepNotes[length - 1];
    bool lastEnabled = sequencerStepEnabled[length - 1];

    for (int i = length - 1; i > 0; i--) {
      sequencerStepNotes[i] = sequencerStepNotes[i - 1];
      sequencerStepEnabled[i] = sequencerStepEnabled[i - 1];
    }

    sequencerStepNotes[0] = lastNote;
    sequencerStepEnabled[0] = lastEnabled;
  } else {
    int firstNote = sequencerStepNotes[0];
    bool firstEnabled = sequencerStepEnabled[0];

    for (int i = 0; i < length - 1; i++) {
      sequencerStepNotes[i] = sequencerStepNotes[i + 1];
      sequencerStepEnabled[i] = sequencerStepEnabled[i + 1];
    }

    sequencerStepNotes[length - 1] = firstNote;
    sequencerStepEnabled[length - 1] = firstEnabled;
  }

  resetSequencerClock();
}

void changeCurrentValue(int delta) {
  MenuAction action = getSelectedAction();
  delta = getEditDeltaForAction(action, delta);
  if (delta == 0) return;

  switch (action) {
    case ACTION_VOICE_WAVE:
    {
      int voice = getCurrentWaveVoiceIndex();
      waveVoiceShape[voice] = wrapIndex(waveVoiceShape[voice] + delta, WAVE_SHAPE_COUNT);
      synthRetriggerRequested = true;
      break;
    }

    case ACTION_VOICE_TUNE:
    {
      int voice = getCurrentWaveVoiceIndex();
      waveVoiceTune[voice] += delta;
      waveVoiceTune[voice] = clampInt(waveVoiceTune[voice], -24, 24);
      break;
    }

    case ACTION_VOICE_FOLD:
    {
      int voice = getCurrentWaveVoiceIndex();
      waveVoiceFold[voice] += delta * 2;
      waveVoiceFold[voice] = clampInt(waveVoiceFold[voice], 0, 100);
      break;
    }

    case ACTION_VOICE_PHASE:
    {
      int voice = getCurrentWaveVoiceIndex();
      waveVoicePhase[voice] += delta * 5;
      waveVoicePhase[voice] = wrapIndex(waveVoicePhase[voice], 360);
      break;
    }

    case ACTION_VOICE_SCOPE:
      break;

    case ACTION_PITCH:
      frequency += delta * 5;
      frequency = clampInt(frequency, 20, 2000);
      break;

    case ACTION_LEVEL:
      volume += delta;
      volume = clampInt(volume, 0, 100);
      break;

    case ACTION_FILTER_POWER:
      filterEnabled = !filterEnabled;
      break;

    case ACTION_FILTER_TYPE:
      filterType = wrapIndex(filterType + delta, FILTER_TYPE_COUNT);
      break;

    case ACTION_FILTER_CUTOFF:
      filterCutoff += delta * 50;
      filterCutoff = clampInt(filterCutoff, FILTER_MIN_CUTOFF_HZ, FILTER_MAX_CUTOFF_HZ);
      break;

    case ACTION_ADSR:
      break;

    case ACTION_ATTACK:
      attack += delta;
      attack = clampInt(attack, 0, 500);
      break;

    case ACTION_DECAY:
      decay += delta;
      decay = clampInt(decay, 0, 2000);
      break;

    case ACTION_SUSTAIN:
      sustain += delta;
      sustain = clampInt(sustain, 0, 100);
      break;

    case ACTION_RELEASE:
      releaseMs += delta * 20;
      releaseMs = clampInt(releaseMs, 0, 3000);
      break;

    case ACTION_FFT_RANGE:
      fftRangeIndex = wrapIndex(fftRangeIndex + delta, FFT_RANGE_COUNT);
      break;

    case ACTION_FFT_WINDOW:
      fftWindow = wrapIndex(fftWindow + delta, FFT_WINDOW_COUNT);
      break;

    case ACTION_FFT_SMOOTH:
      fftSmoothing += delta * 5;
      fftSmoothing = clampInt(fftSmoothing, 0, 95);
      break;

    case ACTION_FFT_HOLD:
      if (delta != 0) {
        fftPeakHold = !fftPeakHold;
      }
      break;

    case ACTION_FFT_BINS:
      fftBinsIndex = wrapIndex(fftBinsIndex + delta, FFT_BIN_COUNT);
      break;

    case ACTION_BT_POWER:
      if (delta != 0) {
        bluetoothEnabled = !bluetoothEnabled;

        if (bluetoothEnabled) {
          startBluetoothAudio();
        } else {
          stopBluetoothAudio();
        }

        lastBtHeaderRefreshMs = 0;
      }
      break;

    case ACTION_AUDIO_TEST:
      waveform += delta;
      waveform = wrapIndex(waveform, WAVE_SHAPE_COUNT);
      break;

    case ACTION_SEQ_START:
      if (delta != 0) {
        startTransport();
      }
      break;

    case ACTION_SEQ_STOP:
      if (delta != 0) {
        stopTransport();
      }
      break;

    case ACTION_SEQ_POWER:
      if (delta != 0) {
        sequencerEnabled = !sequencerEnabled;
        transportRunning = sequencerEnabled;
        resetSequencerClock();
      }
      break;

    case ACTION_SYNTH_VIEW:
      return;

    case ACTION_SEQ_BPM:
      sequencerBpm += delta * 2;
      sequencerBpm = clampInt(sequencerBpm, 40, 240);
      break;

    case ACTION_SEQ_STEP:
      sequencerSelectedStep = wrapIndex(sequencerSelectedStep + delta, getSequencerLength());
      if (!sequencerEnabled && !isMixerLayerEnabled()) {
        sequencerCurrentStep = sequencerSelectedStep;
        resetSequencerClock();
      }
      break;

    case ACTION_SEQ_NOTE:
      sequencerStepNotes[sequencerSelectedStep] += delta;
      sequencerStepNotes[sequencerSelectedStep] = clampInt(sequencerStepNotes[sequencerSelectedStep], SEQUENCER_MIN_NOTE, SEQUENCER_MAX_NOTE);
      break;

    case ACTION_SEQ_LENGTH:
      sequencerLength += delta;
      sequencerLength = clampInt(sequencerLength, 1, SEQUENCER_STEP_COUNT);
      clampSequencerIndexes();
      resetSequencerClock();
      break;

    case ACTION_SEQ_DIRECTION:
      sequencerDirection = wrapIndex(sequencerDirection + delta, SEQUENCER_DIRECTION_COUNT);
      sequencerPingPongForward = true;
      resetSequencerClock();
      break;

    case ACTION_SEQ_GATE:
      sequencerGate += delta * 5;
      sequencerGate = clampInt(sequencerGate, 5, 100);
      break;

    case ACTION_SEQ_TRANSPOSE:
      sequencerTranspose += delta;
      sequencerTranspose = clampInt(sequencerTranspose, SEQUENCER_MIN_TRANSPOSE, SEQUENCER_MAX_TRANSPOSE);
      break;

    case ACTION_SEQ_STEP_ENABLE:
      if (delta != 0) {
        sequencerStepEnabled[sequencerSelectedStep] = !sequencerStepEnabled[sequencerSelectedStep];
      }
      break;

    case ACTION_SEQ_RANDOMIZE:
      if (delta != 0) {
        randomizeSequencerPattern();
      }
      break;

    case ACTION_SEQ_CLEAR:
      if (delta != 0) {
        clearSequencerPattern();
      }
      break;

    case ACTION_SEQ_ROTATE:
      if (delta != 0) {
        rotateSequencerPattern(delta > 0 ? 1 : -1);
      }
      break;

    case ACTION_MIX_SINE:
    case ACTION_MIX_SQUARE:
    case ACTION_MIX_TRIANGLE:
    case ACTION_MIX_SAW:
    {
      int waveIndex = getMixerWaveIndex(action);
      waveVoiceLevels[waveIndex] += delta * 5;
      waveVoiceLevels[waveIndex] = clampInt(waveVoiceLevels[waveIndex], 0, 100);
      break;
    }

    case ACTION_MIX_KICK:
    {
      int previousKickLevel = kickLevel;
      kickLevel += delta * 5;
      kickLevel = clampInt(kickLevel, 0, 100);
      if (previousKickLevel == 0 && kickLevel > 0) {
        triggerKickVoice();
      }
      break;
    }

    case ACTION_KICK_PITCH:
      kickPitch += delta;
      kickPitch = clampInt(kickPitch, 25, 120);
      break;

    case ACTION_KICK_DECAY:
      kickDecay += delta * 10;
      kickDecay = clampInt(kickDecay, 80, 1200);
      break;

    case ACTION_KICK_SWEEP:
      kickSweep += delta * 5;
      kickSweep = clampInt(kickSweep, 0, 260);
      break;

    case ACTION_KICK_CLICK:
      kickClick += delta * 5;
      kickClick = clampInt(kickClick, 0, 100);
      break;

    case ACTION_KICK_DRIVE:
      kickDrive += delta * 5;
      kickDrive = clampInt(kickDrive, 0, 100);
      break;

    case ACTION_DELAY_MIX:
      delayMix = clampInt(delayMix + delta * 5, 0, 100);
      break;

    case ACTION_DELAY_TIME:
      delayTimeMs = clampInt(delayTimeMs + delta * 10, 20, DELAY_MAX_MS);
      break;

    case ACTION_DELAY_FEEDBACK:
      delayFeedback = clampInt(delayFeedback + delta * 5, 0, 90);
      break;

    case ACTION_REVERB_MIX:
      reverbMix = clampInt(reverbMix + delta * 5, 0, 100);
      break;

    case ACTION_REVERB_ROOM:
      reverbRoom = clampInt(reverbRoom + delta * 5, 0, 95);
      break;

    case ACTION_PRESET_SLOT:
      presetSlot = wrapIndex(presetSlot - 1 + delta, PRESET_SLOT_COUNT) + 1;
      presetOperationText = "SELECT";
      break;

    default:
      break;
  }

  valueDirty = true;
}

// =======================
// UI LOGIC
// =======================

void setMode(UiMode newMode) {
  if (newMode == MODE_PREMENU) {
    currentMenuPage = MENU_PAGE_ROOT;
    activeWaveVoiceIndex = 0;
    resetMenuSelection();
  }

  uiMode = newMode;
  fullRedraw = true;
  valueDirty = true;
  lastBtHeaderRefreshMs = 0;
  lastMenuMoveMs = 0;
}

void updateUiLogic() {
  int32_t steps = getEncoderSteps();

  if (uiMode == MODE_BOOT) {
    bool skipRequested = pushEvent || k0Event;
    pushEvent = false;
    k0Event = false;

    if (!bootReady && (skipRequested || millis() - bootStartMs >= bootDurationMs)) {
      bootReady = true;
      bootCompletedAtMs = millis();
      lastBootProgress = -1;
    }

    if (bootReady && millis() - bootCompletedAtMs >= bootCompleteHoldMs) {
      setMode(MODE_PREMENU);
    }

    return;
  }

  if (steps != 0) {
    if (uiMode == MODE_PREMENU) {
      int32_t typeStep = getMenuNavigationStep(steps);

      if (typeStep != 0) {
        selectedType += typeStep;
        wrapSelectedType();

        fullRedraw = false;
      }
    }

    else if (uiMode == MODE_MENU) {
      int32_t menuStep = getMenuNavigationStep(steps);

      if (menuStep != 0) {
        selectedItem += menuStep;
        wrapSelectedMenu();

        fullRedraw = false;
      }
    }

    else if (uiMode == MODE_EDIT) {
      changeCurrentValue(steps);
    }
  }

  if (pushEvent) {
    pushEvent = false;

    if (uiMode == MODE_PREMENU) {
      applyTypeChoice(selectedType);
      setMode(MODE_MENU);
    }

    else if (uiMode == MODE_MENU) {
      MenuAction action = getSelectedAction();

      if (isMenuSubmenuAction(action)) {
        enterSubmenu(action);
      } else if (isImmediateMenuAction(action)) {
        executeImmediateMenuAction(action);
        setMode(MODE_EDIT);
      } else if (action == ACTION_ABOUT) {
        setMode(MODE_ABOUT);
      } else {
        setMode(MODE_EDIT);
      }
    }

    else if (uiMode == MODE_EDIT) {
      setMode(MODE_MENU);
    }

    else if (uiMode == MODE_ABOUT) {
      setMode(MODE_MENU);
    }
  }

  if (k0Event) {
    k0Event = false;

    if (uiMode == MODE_PREMENU) {
      applyTypeChoice(0);
      setMode(MODE_MENU);
    }

    else if (uiMode == MODE_MENU && isInSubmenu()) {
      leaveSubmenu();
    }

    else if (uiMode == MODE_MENU || uiMode == MODE_EDIT || uiMode == MODE_ABOUT) {
      setMode(MODE_PREMENU);
    }
  }
}
