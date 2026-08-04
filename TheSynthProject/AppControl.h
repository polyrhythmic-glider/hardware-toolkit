#pragma once

extern uint16_t neokeyLastRawKeys;
extern uint16_t neokeyStableKeys;
extern uint16_t neokeyLastPrintedRawKeys;
extern uint8_t neokeyLastBoardRawKeys[];
extern uint8_t neokeyBoardFailureCount[];
extern bool neokeyBoardReady[];
extern bool neokeyAnyReady;

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

// getEncoderSteps() restituisce gia uno step per ogni scatto fisico.
// Un'ulteriore soglia a 2 rendeva toggle e selezioni apparentemente bloccati.
const int DISCRETE_EDIT_STEP_THRESHOLD = 1;
int discreteEditAccumulator = 0;
int lastDiscreteEditAction = -1;

void initializeSequencerPitchDefaults() {
  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      sequencerStepPitches[voice][step] = SEQUENCER_DEFAULT_PITCH;
    }
  }
}

void initializeDefaultModulationRouting() {
  for (int source = 0; source < MOD_SOURCE_COUNT; source++) {
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; destination++) {
      modulationRoutes[source][destination].enabled = false;
      modulationRoutes[source][destination].amountPercent = 0;
    }
  }

  // Preserve the former hard-wired amplitude envelope as the initial patch.
  modulationRoutes[MOD_SOURCE_ENV1][MOD_DEST_AMP_LEVEL].enabled = true;
  modulationRoutes[MOD_SOURCE_ENV1][MOD_DEST_AMP_LEVEL].amountPercent = 100;
}

AdsrParameters& getActiveEnvelopeParameters() {
  activeEnvelopeIndex = clampInt(activeEnvelopeIndex, 0, ENVELOPE_COUNT - 1);
  return envelopeParameters[activeEnvelopeIndex];
}

ModRouteSettings& getActiveModulationRoute() {
  activeEnvelopeIndex = clampInt(activeEnvelopeIndex, 0, ENVELOPE_COUNT - 1);
  activeModDestinationIndex = clampInt(activeModDestinationIndex, 0, MOD_DESTINATION_COUNT - 1);
  return modulationRoutes[getEnvelopeModSource(activeEnvelopeIndex)][activeModDestinationIndex];
}

int getModulationDestinationForMenuItem(int itemIndex) {
  int destination = itemIndex - ITEM_MOD_DEST_AMP_LEVEL;
  return clampInt(destination, 0, MOD_DESTINATION_COUNT - 1);
}

const char* getResetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt-wdt";
    case ESP_RST_TASK_WDT: return "task-wdt";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "other";
  }
}

bool isDiscreteEditAction(MenuAction action) {
  switch (action) {
    case ACTION_VOICE_WAVE:
    case ACTION_VOICE_TUNE:
    case ACTION_FILTER_POWER:
    case ACTION_FILTER_TYPE:
    case ACTION_MOD_ROUTE_ENABLED:
    case ACTION_AUDIO_TEST:
    case ACTION_SEQ_START:
    case ACTION_SEQ_STOP:
    case ACTION_SEQ_POWER:
    case ACTION_SYNTH_VIEW:
    case ACTION_SEQ_VOICE:
    case ACTION_SEQ_STEP:
    case ACTION_SEQ_STEP_ENABLE:
    case ACTION_PRESET_SLOT:
    case ACTION_SURFACE_MODE:
    case ACTION_OLED_DEBUG:
    case ACTION_MCP_DEBUG:
    case ACTION_SERIAL_DEBUG:
    case ACTION_CONNECTION_STATUS:
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

bool isWaveVoicePage(MenuPage page) {
  return page == MENU_PAGE_VOICE_1 ||
         page == MENU_PAGE_VOICE_2;
}

int getWaveVoiceIndexForPage(MenuPage page) {
  switch (page) {
    case MENU_PAGE_VOICE_1:
      return 0;

    case MENU_PAGE_VOICE_2:
      return 1;

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
  return action == ACTION_VOICE_1_MENU ||
         action == ACTION_VOICE_2_MENU ||
         action == ACTION_FILTER_MENU ||
         action == ACTION_TRANSPORT_MENU ||
         action == ACTION_ENV_1_MENU ||
         action == ACTION_ENV_2_MENU ||
         action == ACTION_ENV_DESTINATIONS_MENU ||
         action == ACTION_MOD_DESTINATION_MENU;
}

bool isImmediateMenuAction(MenuAction action) {
  return action == ACTION_SEQ_START ||
         action == ACTION_SEQ_STOP ||
         action == ACTION_PRESET_LOAD ||
         action == ACTION_PRESET_SAVE ||
         action == ACTION_OLED_DEBUG ||
         action == ACTION_MCP_DEBUG ||
         action == ACTION_CONNECTION_STATUS;
}

void startTransport() {
  transportRunning = true;
  sequencerEnabled = true;
  resetSequencerClock();

  if (isSerialDiagnosticsEnabled()) {
    Serial.print("[SEQ][TRANSPORT] start bpm=");
    Serial.print(sequencerBpm);
    Serial.print(" length=");
    Serial.print(getSequencerLength());
    Serial.print(" step=");
    Serial.print(sequencerCurrentStep + 1);
    Serial.print(" track=");
    Serial.println(getSequencerTrackLabel(getActiveSequencerTrack()));
  }

  if (kickLevel > 0 && isKickStepEnabled(sequencerCurrentStep)) {
    triggerKickVoice();
  }
}

void stopTransport() {
  transportRunning = false;
  sequencerEnabled = false;
}

void toggleSerialDiagnostics() {
  serialDiagnosticsEnabled = !serialDiagnosticsEnabled;
  if (serialDiagnosticsEnabled) {
    printSerialDiagnosticsSnapshot();
    printConnectionDiagnosticsSnapshot();
  } else {
    Serial.println("[DIAG] Serial diagnostics disabled");
  }
}

void executeImmediateMenuAction(MenuAction action) {
  if (action == ACTION_SEQ_START) {
    if (!transportRunning) startTransport();
  } else if (action == ACTION_SEQ_STOP) {
    stopTransport();
  } else if (action == ACTION_PRESET_LOAD) {
    presetOperationOk = loadCurrentPreset();
    presetOperationText = presetOperationOk ? "LOADED" : "EMPTY";
  } else if (action == ACTION_PRESET_SAVE) {
    presetOperationOk = saveCurrentPreset();
    presetOperationText = presetOperationOk ? "SAVED" : "ERROR";
  } else if (action == ACTION_OLED_DEBUG) {
    ControlSurface::runOledDiagnostics();
  } else if (action == ACTION_MCP_DEBUG) {
    ControlSurface::runMcpDiagnostics();
  } else if (action == ACTION_SERIAL_DEBUG) {
    toggleSerialDiagnostics();
  } else if (action == ACTION_CONNECTION_STATUS) {
    printConnectionDiagnosticsSnapshot();
  }

  valueDirty = true;
  fullRedraw = true;
}

MenuPage getSubmenuPageForAction(MenuAction action) {
  switch (action) {
    case ACTION_VOICE_1_MENU:
      return MENU_PAGE_VOICE_1;

    case ACTION_VOICE_2_MENU:
      return MENU_PAGE_VOICE_2;

    case ACTION_FILTER_MENU:
      return MENU_PAGE_FILTER;

    case ACTION_TRANSPORT_MENU:
      return MENU_PAGE_TRANSPORT;

    case ACTION_ENV_1_MENU:
      return MENU_PAGE_ENV_1;

    case ACTION_ENV_2_MENU:
      return MENU_PAGE_ENV_2;

    case ACTION_ENV_DESTINATIONS_MENU:
      return MENU_PAGE_ENV_DESTINATIONS;

    case ACTION_MOD_DESTINATION_MENU:
      return MENU_PAGE_MOD_DESTINATION;

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

int getMenuItemIndex(int visibleIndex);

void enterSubmenu(MenuAction action) {
  if (action == ACTION_ENV_1_MENU) activeEnvelopeIndex = ENVELOPE_ENV1;
  if (action == ACTION_ENV_2_MENU) activeEnvelopeIndex = ENVELOPE_ENV2;
  if (action == ACTION_MOD_DESTINATION_MENU) {
    activeModDestinationIndex = getModulationDestinationForMenuItem(getMenuItemIndex(selectedItem));
  }

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
  if (currentMenuPage == MENU_PAGE_MOD_DESTINATION) {
    currentMenuPage = MENU_PAGE_ENV_DESTINATIONS;
  } else if (currentMenuPage == MENU_PAGE_ENV_DESTINATIONS) {
    currentMenuPage = activeEnvelopeIndex == ENVELOPE_ENV2 ? MENU_PAGE_ENV_2 : MENU_PAGE_ENV_1;
  } else {
    currentMenuPage = MENU_PAGE_ROOT;
  }
  resetMenuSelection();
  fullRedraw = true;
  valueDirty = true;
}

const uint8_t* getCurrentMenuItems(int* count) {
  if (currentMenuPage == MENU_PAGE_ENV_1 || currentMenuPage == MENU_PAGE_ENV_2) {
    *count = sizeof(MENU_ENV_SOURCE_ITEMS) / sizeof(MENU_ENV_SOURCE_ITEMS[0]);
    return MENU_ENV_SOURCE_ITEMS;
  }

  if (currentMenuPage == MENU_PAGE_ENV_DESTINATIONS) {
    *count = sizeof(MENU_MOD_DESTINATION_ITEMS) / sizeof(MENU_MOD_DESTINATION_ITEMS[0]);
    return MENU_MOD_DESTINATION_ITEMS;
  }

  if (currentMenuPage == MENU_PAGE_MOD_DESTINATION) {
    *count = sizeof(MENU_MOD_ROUTE_ITEMS) / sizeof(MENU_MOD_ROUTE_ITEMS[0]);
    return MENU_MOD_ROUTE_ITEMS;
  }

  if (currentMenuPage == MENU_PAGE_FILTER) {
    *count = sizeof(MENU_FILTER_ITEMS) / sizeof(MENU_FILTER_ITEMS[0]);
    return MENU_FILTER_ITEMS;
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
  else if (currentMenuPage == MENU_PAGE_TRANSPORT) {
    source = "TRANSPORT";
  }
  else if (currentMenuPage == MENU_PAGE_ENV_1 || currentMenuPage == MENU_PAGE_ENV_2) {
    source = activeEnvelopeIndex == ENVELOPE_ENV2 ? "ENV2" : "ENV1";
  }
  else if (currentMenuPage == MENU_PAGE_ENV_DESTINATIONS) {
    source = activeEnvelopeIndex == ENVELOPE_ENV2 ? "ENV2 DESTINATIONS" : "ENV1 DESTINATIONS";
  }
  else if (currentMenuPage == MENU_PAGE_MOD_DESTINATION) {
    source = MOD_DESTINATIONS[activeModDestinationIndex].label;
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
  return action == ACTION_VOICE_1_MENU ||
         action == ACTION_VOICE_2_MENU ||
         action == ACTION_FILTER_MENU ||
         action == ACTION_TRANSPORT_MENU ||
         action == ACTION_ENV_1_MENU ||
         action == ACTION_ENV_2_MENU ||
         action == ACTION_ENV_DESTINATIONS_MENU ||
         action == ACTION_MOD_DESTINATION_MENU ||
         action == ACTION_MOD_ROUTE_ENABLED ||
         action == ACTION_MOD_ROUTE_AMOUNT ||
         action == ACTION_VOICE_WAVE ||
         action == ACTION_VOICE_TUNE ||
         action == ACTION_VOICE_FOLD ||
         action == ACTION_VOICE_DETUNE ||
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
         action == ACTION_SEQ_VOICE ||
         action == ACTION_SEQ_STEP ||
         action == ACTION_SEQ_NOTE ||
         action == ACTION_SEQ_LENGTH ||
         action == ACTION_SEQ_STEP_ENABLE ||
         action == ACTION_MIX_VOICE_1 ||
         action == ACTION_MIX_VOICE_2 ||
         action == ACTION_MIX_KICK ||
         action == ACTION_KICK_PITCH ||
         action == ACTION_KICK_DECAY ||
         action == ACTION_KICK_SWEEP ||
         action == ACTION_KICK_ATTACK ||
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
  if (TYPE_COUNT <= 0) return;

  // When every area fits on screen there is no scrolling window: keeping a
  // circular top offset would redraw the same list starting from Utility when
  // selection wraps backward from Wave Engine.
  if (TYPE_COUNT <= VISIBLE_TYPE_ROWS) {
    selectedType = wrapIndex(selectedType, TYPE_COUNT);
    topType = 0;
    return;
  }

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
}

int getActiveSequencerVoice() {
  activeSequencerVoice = clampInt(activeSequencerVoice, 0, WAVE_VOICE_COUNT - 1);
  return activeSequencerVoice;
}

int getActiveSequencerTrack() {
  activeSequencerTrack = clampInt(activeSequencerTrack, 0, SEQUENCER_TRACK_COUNT - 1);
  if (activeSequencerTrack < WAVE_VOICE_COUNT) {
    activeSequencerVoice = activeSequencerTrack;
  }
  return activeSequencerTrack;
}

bool isActiveSequencerTrackKick() {
  return getActiveSequencerTrack() == SEQUENCER_TRACK_KICK;
}

int getActiveSequencerTrackVoice() {
  int track = getActiveSequencerTrack();
  return track < WAVE_VOICE_COUNT ? track : 0;
}

const char* getSequencerTrackLabel(int track) {
  switch (clampInt(track, 0, SEQUENCER_TRACK_COUNT - 1)) {
    case SEQUENCER_TRACK_VOICE_1: return "V1";
    case SEQUENCER_TRACK_VOICE_2: return "V2";
    default: return "KICK";
  }
}

int getSequencerStepPitch(int voiceIndex, int stepIndex) {
  voiceIndex = clampInt(voiceIndex, 0, WAVE_VOICE_COUNT - 1);
  stepIndex = clampInt(stepIndex, 0, SEQUENCER_STEP_COUNT - 1);
  return clampMidiPitch(sequencerStepPitches[voiceIndex][stepIndex]);
}

float getSequencerResultPitch(int voiceIndex, int stepIndex) {
  voiceIndex = clampInt(voiceIndex, 0, WAVE_VOICE_COUNT - 1);
  return clampFloat(
    getSequencerStepPitch(voiceIndex, stepIndex) +
      waveVoiceTune[voiceIndex] + waveVoiceDetune[voiceIndex] / 100.0f,
    (float)SEQUENCER_MIN_PITCH,
    (float)SEQUENCER_MAX_PITCH
  );
}

float getSequencerResultFrequency(int voiceIndex, int stepIndex) {
  return midiPitchToFrequency(getSequencerResultPitch(voiceIndex, stepIndex));
}

void formatSequencerResultPitch(int voiceIndex, int stepIndex, char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  voiceIndex = clampInt(voiceIndex, 0, WAVE_VOICE_COUNT - 1);
  int coarsePitch = clampMidiPitch(
    getSequencerStepPitch(voiceIndex, stepIndex) + waveVoiceTune[voiceIndex]
  );
  char note[8];
  formatMidiPitch(coarsePitch, note, sizeof(note));
  snprintf(output, outputSize, "%s %+d ct", note, waveVoiceDetune[voiceIndex]);
}

bool isSequencerStepEnabled(int voiceIndex, int stepIndex) {
  voiceIndex = clampInt(voiceIndex, 0, WAVE_VOICE_COUNT - 1);
  stepIndex = clampInt(stepIndex, 0, SEQUENCER_STEP_COUNT - 1);
  return sequencerStepEnabled[voiceIndex][stepIndex];
}

bool isActiveSequencerStepEnabledAtStep(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return false;

  if (isActiveSequencerTrackKick()) return kickStepEnabled[stepIndex];
  return sequencerStepEnabled[getActiveSequencerTrackVoice()][stepIndex];
}

bool isAnySequencerVoiceEnabledAtStep(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return false;

  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    if (waveVoiceLevels[voice] > 0 && sequencerStepEnabled[voice][stepIndex]) {
      return true;
    }
  }

  return false;
}

void setWaveVoiceShapeForTrack(int voiceIndex, int shapeIndex) {
  voiceIndex = clampInt(voiceIndex, 0, WAVE_VOICE_COUNT - 1);
  shapeIndex = wrapIndex(shapeIndex, WAVE_SHAPE_COUNT);
  waveVoiceShape[voiceIndex] = shapeIndex;

  // The audio task crossfades shapes; restarting the envelope at every encoder
  // detent only adds a transient while browsing waveforms.
}

void setSequencerStepEnabled(int voiceIndex, int stepIndex, bool enabled) {
  voiceIndex = clampInt(voiceIndex, 0, WAVE_VOICE_COUNT - 1);
  stepIndex = clampInt(stepIndex, 0, SEQUENCER_STEP_COUNT - 1);
  sequencerStepEnabled[voiceIndex][stepIndex] = enabled;
}

int getSelectedSequencerStepPitch() {
  return getSequencerStepPitch(getActiveSequencerTrackVoice(), sequencerSelectedStep);
}

bool isSelectedSequencerStepEnabled() {
  if (isActiveSequencerTrackKick()) return kickStepEnabled[clampInt(sequencerSelectedStep, 0, SEQUENCER_STEP_COUNT - 1)];
  return isSequencerStepEnabled(getActiveSequencerTrackVoice(), sequencerSelectedStep);
}

bool isKickStepEnabled(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return false;
  return kickStepEnabled[stepIndex];
}

void printSerialDiagnosticsSnapshot() {
  serialDebugBegin("SYSTEM / SYNTH / SEQUENCER");

  Serial.print("Uptime ms: ");
  Serial.println(millis());
  Serial.print("Reset reason: ");
  Serial.print((int)esp_reset_reason());
  Serial.print(" (");
  Serial.print(getResetReasonText(esp_reset_reason()));
  Serial.println(")");
  Serial.print("CPU MHz: ");
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print("Heap free/min/largest: ");
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_8BIT));
  Serial.print("/");
  Serial.print(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
  Serial.print("/");
  Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

  Serial.println("Audio backend: DAISY SEED (UART details follow in connection summary)");

  Serial.print("Transport: ");
  Serial.print(transportRunning ? "RUN" : "STOP");
  Serial.print("  sequencer: ");
  Serial.print(sequencerEnabled ? "ON" : "OFF");
  Serial.print("  BPM: ");
  Serial.print(sequencerBpm);
  Serial.print("  length: ");
  Serial.println(getSequencerLength());
  Serial.print("Track/current/selected: ");
  Serial.print(getSequencerTrackLabel(getActiveSequencerTrack()));
  Serial.print("/");
  Serial.print((int)sequencerCurrentStep + 1);
  Serial.print("/");
  Serial.println(sequencerSelectedStep + 1);
  Serial.println("Clock owner: DAISY (playhead received over UART)");

  Serial.print("Output/V1/V2/kick: ");
  Serial.print(volume);
  Serial.print("/");
  Serial.print(waveVoiceLevels[0]);
  Serial.print("/");
  Serial.print(waveVoiceLevels[1]);
  Serial.print("/");
  Serial.println(kickLevel);
  Serial.print("V1 shape/tune/fold/detune: ");
  Serial.print(waveVoiceShape[0]);
  Serial.print("/");
  Serial.print(waveVoiceTune[0]);
  Serial.print("/");
  Serial.print(waveVoiceFold[0]);
  Serial.print("/");
  Serial.println(waveVoiceDetune[0]);
  Serial.print("V2 shape/tune/fold/detune: ");
  Serial.print(waveVoiceShape[1]);
  Serial.print("/");
  Serial.print(waveVoiceTune[1]);
  Serial.print("/");
  Serial.print(waveVoiceFold[1]);
  Serial.print("/");
  Serial.println(waveVoiceDetune[1]);
  Serial.print("Kick 909 tune/attack/decay/sweep/drive: ");
  Serial.print(kickPitch);
  Serial.print("/");
  Serial.print(kickAttack);
  Serial.print("/");
  Serial.print(kickDecay);
  Serial.print("/");
  Serial.print(kickSweep);
  Serial.print("/");
  Serial.println(kickDrive);
  for (int envelope = 0; envelope < ENVELOPE_COUNT; envelope++) {
    const AdsrParameters& parameters = envelopeParameters[envelope];
    Serial.print("ENV");
    Serial.print(envelope + 1);
    Serial.print(" attack/decay/sustain/release ms: ");
    Serial.print(parameters.attackMs);
    Serial.print("/");
    Serial.print(parameters.decayMs);
    Serial.print("/");
    Serial.print(parameters.sustainPercent);
    Serial.print("/");
    Serial.println(parameters.releaseMs);

    ModSourceId source = getEnvelopeModSource(envelope);
    Serial.print("ENV");
    Serial.print(envelope + 1);
    Serial.print(" routes:");
    bool anyRoute = false;
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; destination++) {
      const ModRouteSettings& route = modulationRoutes[source][destination];
      if (!route.enabled) continue;
      Serial.print(" ");
      Serial.print(MOD_DESTINATIONS[destination].tag);
      Serial.print("=");
      if (route.amountPercent >= 0) Serial.print("+");
      Serial.print((int)route.amountPercent);
      Serial.print("%");
      anyRoute = true;
    }
    if (!anyRoute) Serial.print(" none");
    Serial.println();
  }
  Serial.print("Filter enabled/type/cutoff Hz: ");
  Serial.print(filterEnabled ? "ON" : "OFF");
  Serial.print("/");
  Serial.print(filterType);
  Serial.print("/");
  Serial.println(filterCutoff);
  Serial.print("Delay mix/time/feedback: ");
  Serial.print(delayMix);
  Serial.print("/");
  Serial.print(delayTimeMs);
  Serial.print("/");
  Serial.println(delayFeedback);
  Serial.print("Reverb mix/room  preset slot: ");
  Serial.print(reverbMix);
  Serial.print("/");
  Serial.print(reverbRoom);
  Serial.print("  ");
  Serial.println(presetSlot);

  Serial.println("Pattern: step | V1 enabled note | V2 enabled note | kick");
  for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
    if (step + 1 < 10) Serial.print('0');
    Serial.print(step + 1);
    Serial.print(" | ");
    Serial.print(sequencerStepEnabled[0][step] ? "ON  " : "OFF ");
    char voice1Pitch[8];
    formatMidiPitch(getSequencerStepPitch(0, step), voice1Pitch, sizeof(voice1Pitch));
    Serial.print(voice1Pitch);
    Serial.print(" | ");
    Serial.print(sequencerStepEnabled[1][step] ? "ON  " : "OFF ");
    char voice2Pitch[8];
    formatMidiPitch(getSequencerStepPitch(1, step), voice2Pitch, sizeof(voice2Pitch));
    Serial.print(voice2Pitch);
    Serial.print(" | ");
    Serial.println(kickStepEnabled[step] ? "HIT" : "REST");
  }

  Serial.print("NeoKey raw/stable: 0x");
  Serial.print(neokeyLastRawKeys, HEX);
  Serial.print("/0x");
  Serial.println(neokeyStableKeys, HEX);
  Serial.print("NeoKey any ready: ");
  Serial.println(neokeyAnyReady ? "YES" : "NO");
  for (uint8_t board = 0; board < 4; board++) {
    Serial.print("NeoKey 0x");
    Serial.print(0x30 + board, HEX);
    Serial.print(" ready/raw/failures: ");
    Serial.print(neokeyBoardReady[board] ? "YES" : "NO");
    Serial.print("/0x");
    Serial.print(neokeyLastBoardRawKeys[board], HEX);
    Serial.print("/");
    Serial.println(neokeyBoardFailureCount[board]);
  }
  serialDebugEnd("SYSTEM / SYNTH / SEQUENCER");

  ControlSurface::runOledDiagnostics();
  ControlSurface::runMcpDiagnostics();
}

const char* getValueText(int itemIndex) {
  static char buffer[24];
  MenuAction action = menuItems[itemIndex].action;

  switch (action) {
    case ACTION_VOICE_1_MENU:
    case ACTION_VOICE_2_MENU:
    case ACTION_FILTER_MENU:
    case ACTION_TRANSPORT_MENU:
    case ACTION_ENV_1_MENU:
    case ACTION_ENV_2_MENU:
    case ACTION_ENV_DESTINATIONS_MENU:
      snprintf(buffer, sizeof(buffer), "OPEN");
      break;

    case ACTION_MOD_DESTINATION_MENU:
    {
      int destination = getModulationDestinationForMenuItem(itemIndex);
      ModRouteSettings& route = modulationRoutes[getEnvelopeModSource(activeEnvelopeIndex)][destination];
      if (!route.enabled) snprintf(buffer, sizeof(buffer), "OFF");
      else snprintf(buffer, sizeof(buffer), "%+d%%", (int)route.amountPercent);
      break;
    }

    case ACTION_MOD_ROUTE_ENABLED:
      snprintf(buffer, sizeof(buffer), "%s", getActiveModulationRoute().enabled ? "ON" : "OFF");
      break;

    case ACTION_MOD_ROUTE_AMOUNT:
      snprintf(buffer, sizeof(buffer), "%+d%%", (int)getActiveModulationRoute().amountPercent);
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

    case ACTION_VOICE_DETUNE:
      snprintf(buffer, sizeof(buffer), "%+dct", waveVoiceDetune[getCurrentWaveVoiceIndex()]);
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
      snprintf(buffer, sizeof(buffer), "%dms", getActiveEnvelopeParameters().attackMs);
      break;

    case ACTION_DECAY:
      snprintf(buffer, sizeof(buffer), "%dms", getActiveEnvelopeParameters().decayMs);
      break;

    case ACTION_SUSTAIN:
      snprintf(buffer, sizeof(buffer), "S%d%%", getActiveEnvelopeParameters().sustainPercent);
      break;

    case ACTION_RELEASE:
      snprintf(buffer, sizeof(buffer), "%dms", getActiveEnvelopeParameters().releaseMs);
      break;

    case ACTION_SCOPE:
      snprintf(buffer, sizeof(buffer), "Live");
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

    case ACTION_SEQ_VOICE:
      snprintf(buffer, sizeof(buffer), "%s", getSequencerTrackLabel(getActiveSequencerTrack()));
      break;

    case ACTION_SEQ_STEP:
      snprintf(buffer, sizeof(buffer), "S%d", sequencerSelectedStep + 1);
      break;

    case ACTION_SEQ_NOTE:
      if (isActiveSequencerTrackKick()) {
        snprintf(buffer, sizeof(buffer), "%s", isSelectedSequencerStepEnabled() ? "HIT" : "REST");
      } else {
        formatMidiPitch(getSelectedSequencerStepPitch(), buffer, sizeof(buffer));
      }
      break;

    case ACTION_SEQ_LENGTH:
      snprintf(buffer, sizeof(buffer), "%d", sequencerLength);
      break;

    case ACTION_SEQ_STEP_ENABLE:
      snprintf(buffer, sizeof(buffer), "%s", isSelectedSequencerStepEnabled() ? "ON" : "REST");
      break;

    case ACTION_MIX_VOICE_1:
    case ACTION_MIX_VOICE_2:
    {
      int waveIndex = getMixerWaveIndex(action);
      snprintf(buffer, sizeof(buffer), "%d%%", waveIndex >= 0 ? waveVoiceLevels[waveIndex] : 0);
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

    case ACTION_KICK_ATTACK:
      snprintf(buffer, sizeof(buffer), "%d%%", kickAttack);
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

    case ACTION_SURFACE_MODE:
      snprintf(buffer, sizeof(buffer), "%s", ControlSurface::getModeName());
      break;

    case ACTION_OLED_DEBUG:
      snprintf(buffer, sizeof(buffer), "ACK %X INIT %X",
               ControlSurface::getOledDetectedMask(),
               ControlSurface::getOledReadyMask());
      break;

    case ACTION_MCP_DEBUG:
      snprintf(buffer, sizeof(buffer), "MCP %s RAW %04X",
               ControlSurface::isEncoderExpanderReady() ? "OK" : "MISS",
               ControlSurface::getMcpRawPorts());
      break;

    case ACTION_NEOKEY_DEBUG:
      snprintf(buffer, sizeof(buffer), "RAW %04X", neokeyLastRawKeys);
      break;

    case ACTION_SERIAL_DEBUG:
      snprintf(buffer, sizeof(buffer), "%s", isSerialDiagnosticsEnabled() ? "ON" : "OFF");
      break;

    case ACTION_CONNECTION_STATUS:
    {
      uint8_t problems = getConnectionProblemCount();
      if (problems == 0) snprintf(buffer, sizeof(buffer), "ALL OK");
      else snprintf(buffer, sizeof(buffer), "CHECK %u", problems);
      break;
    }

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
  activeSequencerTrack = clampInt(activeSequencerTrack, 0, SEQUENCER_TRACK_COUNT - 1);
  activeSequencerVoice = clampInt(activeSequencerVoice, 0, WAVE_VOICE_COUNT - 1);
  if (activeSequencerTrack < WAVE_VOICE_COUNT) {
    activeSequencerVoice = activeSequencerTrack;
  }

  if (sequencerSelectedStep >= length) {
    sequencerSelectedStep = length - 1;
  }

  if (sequencerCurrentStep >= length) {
    sequencerCurrentStep = 0;
  }

  if (sequencerSelectedStep < 0) sequencerSelectedStep = 0;
  if (sequencerCurrentStep < 0) sequencerCurrentStep = 0;
}

void changeCurrentValue(int delta) {
  MenuAction action = getSelectedAction();
  delta = getEditDeltaForAction(action, delta);
  if (delta == 0) return;

  switch (action) {
    case ACTION_VOICE_WAVE:
    {
      int voice = getCurrentWaveVoiceIndex();
      setWaveVoiceShapeForTrack(voice, waveVoiceShape[voice] + delta);
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

    case ACTION_VOICE_DETUNE:
    {
      int voice = getCurrentWaveVoiceIndex();
      waveVoiceDetune[voice] += delta;
      waveVoiceDetune[voice] = clampInt(waveVoiceDetune[voice], -50, 50);
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
      getActiveEnvelopeParameters().attackMs += delta;
      getActiveEnvelopeParameters().attackMs = clampInt(getActiveEnvelopeParameters().attackMs, 0, 500);
      break;

    case ACTION_DECAY:
      getActiveEnvelopeParameters().decayMs += delta;
      getActiveEnvelopeParameters().decayMs = clampInt(getActiveEnvelopeParameters().decayMs, 0, 2000);
      break;

    case ACTION_SUSTAIN:
      getActiveEnvelopeParameters().sustainPercent += delta;
      getActiveEnvelopeParameters().sustainPercent = clampInt(getActiveEnvelopeParameters().sustainPercent, 0, 100);
      break;

    case ACTION_RELEASE:
      getActiveEnvelopeParameters().releaseMs += delta * 20;
      getActiveEnvelopeParameters().releaseMs = clampInt(getActiveEnvelopeParameters().releaseMs, 0, 3000);
      break;

    case ACTION_MOD_ROUTE_ENABLED:
      getActiveModulationRoute().enabled = !getActiveModulationRoute().enabled;
      break;

    case ACTION_MOD_ROUTE_AMOUNT:
    {
      int amount = (int)getActiveModulationRoute().amountPercent + delta;
      getActiveModulationRoute().amountPercent = (int8_t)clampModulationAmount(amount);
      break;
    }

    case ACTION_AUDIO_TEST:
      return;

    case ACTION_SEQ_START:
      if (delta != 0 && !transportRunning) {
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
        sequencerEnabled = true;
        if (!transportRunning) {
          transportRunning = true;
          resetSequencerClock();
        }
      }
      break;

    case ACTION_SYNTH_VIEW:
      activeSequencerTrack = wrapIndex(activeSequencerTrack + delta, SEQUENCER_TRACK_COUNT);
      if (activeSequencerTrack < WAVE_VOICE_COUNT) {
        activeSequencerVoice = activeSequencerTrack;
        activeWaveVoiceIndex = activeSequencerTrack;
      }
      ControlSurface::markDirty();
      break;

    case ACTION_SEQ_BPM:
      sequencerBpm += delta * 2;
      sequencerBpm = clampInt(sequencerBpm, 40, 240);
      break;

    case ACTION_SEQ_VOICE:
      activeSequencerTrack = wrapIndex(activeSequencerTrack + delta, SEQUENCER_TRACK_COUNT);
      if (activeSequencerTrack < WAVE_VOICE_COUNT) {
        activeSequencerVoice = activeSequencerTrack;
      }
      if (!transportRunning) resetSequencerClock();
      break;

    case ACTION_SEQ_STEP:
      sequencerSelectedStep = wrapIndex(sequencerSelectedStep + delta, getSequencerLength());
      if (!transportRunning) {
        sequencerCurrentStep = sequencerSelectedStep;
        resetSequencerClock();
      }
      break;

    case ACTION_SEQ_NOTE:
    {
      if (isActiveSequencerTrackKick()) {
        kickStepEnabled[sequencerSelectedStep] = delta > 0;
        if (kickLevel == 0 && kickStepEnabled[sequencerSelectedStep]) kickLevel = 80;
        if (kickStepEnabled[sequencerSelectedStep]) triggerKickVoice();
      } else {
        int voice = getActiveSequencerTrackVoice();
        sequencerStepPitches[voice][sequencerSelectedStep] = clampMidiPitch(
          sequencerStepPitches[voice][sequencerSelectedStep] + delta
        );
        sequencerStepEnabled[voice][sequencerSelectedStep] = true;
      }
      break;
    }

    case ACTION_SEQ_LENGTH:
      sequencerLength += delta;
      sequencerLength = clampInt(sequencerLength, 1, SEQUENCER_STEP_COUNT);
      clampSequencerIndexes();
      if (!transportRunning) resetSequencerClock();
      break;

    case ACTION_SEQ_STEP_ENABLE:
      if (delta != 0) {
        if (isActiveSequencerTrackKick()) {
          kickStepEnabled[sequencerSelectedStep] = !kickStepEnabled[sequencerSelectedStep];
          if (kickLevel == 0 && kickStepEnabled[sequencerSelectedStep]) kickLevel = 80;
          if (kickStepEnabled[sequencerSelectedStep]) triggerKickVoice();
        } else {
          int voice = getActiveSequencerTrackVoice();
          sequencerStepEnabled[voice][sequencerSelectedStep] = !sequencerStepEnabled[voice][sequencerSelectedStep];
        }
      }
      break;

    case ACTION_MIX_VOICE_1:
    case ACTION_MIX_VOICE_2:
    {
      int waveIndex = getMixerWaveIndex(action);
      if (waveIndex < 0) break;
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

    case ACTION_KICK_ATTACK:
      kickAttack += delta * 5;
      kickAttack = clampInt(kickAttack, 0, 100);
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

    case ACTION_SURFACE_MODE:
      ControlSurface::stepMode(delta);
      break;

    case ACTION_OLED_DEBUG:
      ControlSurface::runOledDiagnostics();
      break;

    case ACTION_MCP_DEBUG:
      ControlSurface::runMcpDiagnostics();
      break;

    case ACTION_NEOKEY_DEBUG:
      neokeyLastPrintedRawKeys = 0xFFFF;
      break;

    case ACTION_SERIAL_DEBUG:
      toggleSerialDiagnostics();
      break;

    case ACTION_CONNECTION_STATUS:
      ControlSurface::runOledDiagnostics();
      ControlSurface::runMcpDiagnostics();
      printConnectionDiagnosticsSnapshot();
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
    wrapSelectedType();
  }

  uiMode = newMode;
  fullRedraw = true;
  valueDirty = true;
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
    if (digitalRead(ENC_SW) == LOW && millis() - pushEventMs < 220) {
      return;
    }

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
      if (getSelectedAction() == ACTION_SYNTH_VIEW) {
        if (transportRunning && sequencerEnabled) {
          stopTransport();
        } else {
          startTransport();
        }
        valueDirty = true;
        fullRedraw = false;
      } else if (getSelectedAction() == ACTION_SERIAL_DEBUG) {
        toggleSerialDiagnostics();
        valueDirty = true;
        fullRedraw = false;
      } else {
        setMode(MODE_MENU);
      }
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
