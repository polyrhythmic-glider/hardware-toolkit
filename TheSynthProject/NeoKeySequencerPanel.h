#pragma once

#include <Wire.h>
#include "Adafruit_NeoKey_1x4.h"
#include "seesaw_neopixel.h"
#include "HardwarePins.h"
#include "SerialDebug.h"

// 4x Adafruit NeoKey 1x4 QT concatenati sul canale 4 del PCA9548A.
// Ordine fisico/logico da sinistra a destra: 0x30, 0x31, 0x32, 0x33.

const uint8_t NEOKEY_BOARD_COUNT = 4;
const uint8_t NEOKEY_KEYS_PER_BOARD = 4;
const uint8_t NEOKEY_TOTAL_KEYS = NEOKEY_BOARD_COUNT * NEOKEY_KEYS_PER_BOARD;
const uint8_t NEOKEY_BOARD_ADDRESSES[NEOKEY_BOARD_COUNT] = { 0x30, 0x31, 0x32, 0x33 };
const uint8_t NEOKEY_LOCAL_KEY_MAP[NEOKEY_KEYS_PER_BOARD] = { 0, 1, 2, 3 };
const uint32_t NEOKEY_KEY_POLL_MS = 10;
const uint32_t NEOKEY_KEY_DEBOUNCE_MS = 20;
const uint32_t NEOKEY_REFRESH_MS = 25;
const uint32_t NEOKEY_RECONNECT_MS = 1800;
const uint8_t NEOKEY_BOOT_RETRY_COUNT = 4;
const uint8_t NEOKEY_RUNTIME_FAILURE_LIMIT = 3;
const uint8_t NEOKEY_BRIGHTNESS = 36;

Adafruit_NeoKey_1x4 neokeyBoards[NEOKEY_BOARD_COUNT] = {
  Adafruit_NeoKey_1x4(0x30),
  Adafruit_NeoKey_1x4(0x31),
  Adafruit_NeoKey_1x4(0x32),
  Adafruit_NeoKey_1x4(0x33),
};

bool neokeyBoardReady[NEOKEY_BOARD_COUNT] = { false };
bool neokeyAnyReady = false;
uint16_t neokeyLastRawKeys = 0;
uint16_t neokeyStableKeys = 0;
uint32_t neokeyLastPollMs = 0;
uint32_t neokeyLastRefreshMs = 0;
uint32_t neokeyLastReconnectMs = 0;
uint16_t neokeyLastPrintedRawKeys = 0xFFFF;
uint8_t neokeyLastBoardRawKeys[NEOKEY_BOARD_COUNT] = { 0, 0, 0, 0 };
uint8_t neokeyBoardFailureCount[NEOKEY_BOARD_COUNT] = { 0 };
bool neokeyButtonLastRaw[NEOKEY_TOTAL_KEYS] = { false };
bool neokeyButtonStable[NEOKEY_TOTAL_KEYS] = { false };
uint32_t neokeyButtonChangedAtMs[NEOKEY_TOTAL_KEYS] = { 0 };
int neokeyLastTrack = -1;
int neokeyLastPlayStep = -1;
int neokeyLastPage = -1;
uint64_t neokeyLastPatternSignature = 0xFFFFFFFFFFFFFFFFULL;
bool neokeyLastSequencerRunning = false;
uint32_t neokeyLastStepColors[NEOKEY_TOTAL_KEYS] = { 0 };
bool neokeyStepColorCacheValid = false;

bool neokeySelectPcaChannel() {
  Wire.setClock(NEOKEY_I2C_CLOCK_HZ);
  Wire.beginTransmission(SURFACE_PCA9548A_ADDRESS);
  Wire.write((uint8_t)(1U << NEOKEY_PCA_CHANNEL));
  return Wire.endTransmission(true) == 0;
}

void neokeyRestoreDisplayBusClock() {
  Wire.setClock(SURFACE_DISPLAY_I2C_CLOCK_HZ);
}

bool neokeyPingAddress(uint8_t address) {
  if (!neokeySelectPcaChannel()) {
    neokeyRestoreDisplayBusClock();
    return false;
  }

  Wire.beginTransmission(address);
  bool present = Wire.endTransmission(true) == 0;
  neokeyRestoreDisplayBusClock();
  return present;
}

bool neokeyBeginBoard(uint8_t board) {
  if (board >= NEOKEY_BOARD_COUNT || !neokeySelectPcaChannel()) {
    neokeyRestoreDisplayBusClock();
    return false;
  }

  uint8_t address = NEOKEY_BOARD_ADDRESSES[board];
  bool ready = neokeyBoards[board].begin(address);

  if (ready) {
    neokeyBoards[board].pixels.setBrightness(NEOKEY_BRIGHTNESS);
    neokeyBoards[board].pixels.clear();
    neokeyBoards[board].pixels.show();
  }

  neokeyRestoreDisplayBusClock();
  return ready;
}

void neokeyRecomputeReadyState() {
  neokeyAnyReady = false;
  for (uint8_t board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    neokeyAnyReady = neokeyAnyReady || neokeyBoardReady[board];
  }
}

bool neokeySequencerControlActive() {
  if (!DaisyBridge::isConnected()) return false;
  if (uiMode == MODE_EDIT && getSelectedAction() == ACTION_NEOKEY_DEBUG) return false;
  return uiMode != MODE_BOOT && uiMode != MODE_PREMENU && uiMode != MODE_ABOUT;
}

bool neokeySequencerRunning() {
  return transportRunning && sequencerEnabled;
}

uint32_t neokeyColor(uint8_t r, uint8_t g, uint8_t b) {
  return seesaw_NeoPixel::Color(r, g, b);
}

uint32_t neokeyStepColor(int track, bool active, bool playhead) {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  if (active) {
    if (track == SEQUENCER_TRACK_VOICE_1) r = 190;
    else if (track == SEQUENCER_TRACK_VOICE_2) b = 180;
    else g = 150;
  }
  // The NeoKey line has only two states: stored note and moving playhead.
  // Selection belongs to the editor and is never drawn on these LEDs.
  if (playhead) return neokeyColor(255, 185, 24);
  return neokeyColor(r, g, b);
}

void neokeySetStepPixel(int stepIndex, uint32_t color) {
  if (stepIndex < 0 || stepIndex >= NEOKEY_TOTAL_KEYS) return;

  int board = stepIndex / NEOKEY_KEYS_PER_BOARD;
  int localKey = NEOKEY_LOCAL_KEY_MAP[stepIndex % NEOKEY_KEYS_PER_BOARD];

  if (!neokeyBoardReady[board]) return;

  neokeyBoards[board].pixels.setPixelColor(localKey, color);
}

void neokeyShowAll() {
  if (!neokeySelectPcaChannel()) {
    neokeyRestoreDisplayBusClock();
    return;
  }

  for (int board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    if (neokeyBoardReady[board]) {
      neokeyBoards[board].pixels.show();
    }
  }

  neokeyRestoreDisplayBusClock();
}

void neokeyClearAll() {
  for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
    neokeySetStepPixel(i, 0);
  }

  neokeyShowAll();
  neokeyStepColorCacheValid = false;
}

void neokeyMarkSequencerDirty() {
  valueDirty = true;
  fullRedraw = false;
}

void neokeySelectStep(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return;

  sequencerSelectedStep = stepIndex;

  if (!transportRunning) {
    sequencerCurrentStep = sequencerSelectedStep;
    resetSequencerClock();
  }
}

void neokeyTogglePressedSteps(uint16_t pressed) {
  if (pressed == 0) return;
  int track = getActiveSequencerTrack();
  bool kickTrack = track == SEQUENCER_TRACK_KICK;
  int voice = getActiveSequencerTrackVoice();

  if (!kickTrack) {
    activeSequencerVoice = voice;
  }

  int pageStart = getSequencerPageStart();
  for (int localStep = 0; localStep < NEOKEY_TOTAL_KEYS; localStep++) {
    if ((pressed & ((uint16_t)1U << localStep)) == 0) continue;
    int step = pageStart + localStep;
    if (step >= getSequencerLength()) continue;
    if (isPatternEditor() && step != sequencerSelectedStep) {
      neokeySelectStep(step);
      continue;
    }
    if (kickTrack) {
      kickStepEnabled[step] = !kickStepEnabled[step];
      if (kickLevel == 0 && kickStepEnabled[step]) {
        kickLevel = 80;
      }
      if (kickStepEnabled[step]) {
        triggerKickVoice();
      }
    } else {
      sequencerStepEnabled[voice][step] = !sequencerStepEnabled[voice][step];
    }
  }

  // In Pattern, a different key selects; pressing the selected key toggles.
  // Outside the editor, keys retain their immediate performance toggles.
  neokeyMarkSequencerDirty();
}

uint16_t neokeyReadButtonsRaw() {
  uint16_t rawKeys = 0;
  if (!neokeySelectPcaChannel()) {
    neokeyRestoreDisplayBusClock();
    return rawKeys;
  }

  for (int board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    if (!neokeyBoardReady[board]) continue;

    uint8_t boardKeys = neokeyBoards[board].read() & 0x0F;
    uint8_t mappedKeys = 0;

    for (int i = 0; i < NEOKEY_KEYS_PER_BOARD; i++) {
      if (boardKeys & (1 << NEOKEY_LOCAL_KEY_MAP[i])) {
        mappedKeys |= (1 << i);
      }
    }

    neokeyLastBoardRawKeys[board] = mappedKeys;
    rawKeys |= (uint16_t)mappedKeys << (board * NEOKEY_KEYS_PER_BOARD);
  }

  neokeyRestoreDisplayBusClock();
  return rawKeys;
}

void neokeyPrintRawChange(uint16_t rawKeys) {
  if (!(uiMode == MODE_EDIT && getSelectedAction() == ACTION_NEOKEY_DEBUG)) return;
  if (rawKeys == neokeyLastPrintedRawKeys) return;
  neokeyLastPrintedRawKeys = rawKeys;
  valueDirty = true;
  fullRedraw = false;

  Serial.print("[NEOKEY][RAW] 0x");
  if (rawKeys < 0x1000) Serial.print('0');
  if (rawKeys < 0x0100) Serial.print('0');
  if (rawKeys < 0x0010) Serial.print('0');
  Serial.print(rawKeys, HEX);
  Serial.print("  b0=");
  Serial.print(neokeyLastBoardRawKeys[0], HEX);
  Serial.print(" b1=");
  Serial.print(neokeyLastBoardRawKeys[1], HEX);
  Serial.print(" b2=");
  Serial.print(neokeyLastBoardRawKeys[2], HEX);
  Serial.print(" b3=");
  Serial.println(neokeyLastBoardRawKeys[3], HEX);
}

void neokeySyncButtonDebounce(uint16_t rawKeys, uint32_t now);

void neokeyMaintainConnections(uint32_t now) {
  if (now - neokeyLastReconnectMs < NEOKEY_RECONNECT_MS) return;
  neokeyLastReconnectMs = now;

  bool stateChanged = false;

  for (uint8_t board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    uint8_t address = NEOKEY_BOARD_ADDRESSES[board];

    if (neokeyBoardReady[board]) {
      if (neokeyPingAddress(address)) {
        neokeyBoardFailureCount[board] = 0;
      } else if (++neokeyBoardFailureCount[board] >= NEOKEY_RUNTIME_FAILURE_LIMIT) {
        neokeyBoardReady[board] = false;
        neokeyBoardFailureCount[board] = 0;
        stateChanged = true;
        if (isSerialDiagnosticsEnabled()) {
          Serial.print("[EVENT][NEOKEY] connection lost at 0x");
          Serial.println(address, HEX);
        }
      }
      continue;
    }

    if (neokeyBeginBoard(board)) {
      neokeyBoardReady[board] = true;
      neokeyBoardFailureCount[board] = 0;
      stateChanged = true;
      if (isSerialDiagnosticsEnabled()) {
        Serial.print("[EVENT][NEOKEY] recovered at 0x");
        Serial.println(address, HEX);
      }
    }
  }

  neokeyRecomputeReadyState();

  if (stateChanged) {
    neokeyLastPlayStep = -1;
    neokeyLastPage = -1;
    neokeyLastPatternSignature = 0xFFFFFFFFFFFFFFFFULL;
    neokeyStepColorCacheValid = false;
    neokeySyncButtonDebounce(neokeyAnyReady ? neokeyReadButtonsRaw() : 0, now);
  }
}

void neokeySyncButtonDebounce(uint16_t rawKeys, uint32_t now) {
  neokeyLastRawKeys = rawKeys;
  neokeyStableKeys = rawKeys;

  for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
    bool raw = (rawKeys & ((uint16_t)1U << i)) != 0;
    neokeyButtonLastRaw[i] = raw;
    neokeyButtonStable[i] = raw;
    neokeyButtonChangedAtMs[i] = now;
  }
}

void pollNeoKeySequencerKeys() {
  uint32_t now = millis();
  neokeyMaintainConnections(now);
  if (!neokeyAnyReady) return;

  if (now - neokeyLastPollMs < NEOKEY_KEY_POLL_MS) return;
  neokeyLastPollMs = now;

  uint16_t rawKeys = neokeyReadButtonsRaw();
  neokeyLastRawKeys = rawKeys;
  neokeyPrintRawChange(rawKeys);

  if (rawKeys != 0 && digitalRead(ENC_SW) == LOW) {
    pushEvent = false;
  }
  if (rawKeys != 0 && digitalRead(BTN_K0) == LOW) {
    k0Event = false;
  }

  if (!neokeySequencerControlActive()) {
    neokeySyncButtonDebounce(rawKeys, now);
    return;
  }

  uint16_t stableMask = 0;
  uint16_t pressed = 0;

  for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
    bool raw = (rawKeys & ((uint16_t)1U << i)) != 0;

    if (raw != neokeyButtonLastRaw[i]) {
      neokeyButtonLastRaw[i] = raw;
      neokeyButtonChangedAtMs[i] = now;
    }

    if (now - neokeyButtonChangedAtMs[i] >= NEOKEY_KEY_DEBOUNCE_MS &&
        raw != neokeyButtonStable[i]) {
      neokeyButtonStable[i] = raw;

      if (raw) {
        pressed |= (uint16_t)1U << i;
      }
    }

    if (neokeyButtonStable[i]) {
      stableMask |= (uint16_t)1U << i;
    }
  }

  neokeyStableKeys = stableMask;

  neokeyTogglePressedSteps(pressed);
}

uint64_t neokeyBuildPatternSignature() {
  uint64_t mask = 0;
  int track = getActiveSequencerTrack();

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    if (isSequencerTrackStepEnabled(track, i)) mask |= (uint64_t)1ULL << i;
  }

  return mask;
}

void refreshNeoKeySequencerPanel() {
  if (!neokeyAnyReady) return;

  uint32_t now = millis();
  int track = getActiveSequencerTrack();
  int playStep = clampInt((int)sequencerCurrentStep, 0, getSequencerLength() - 1);
  bool running = neokeySequencerRunning();
  int page = getSequencerPage();
  int pageStart = getSequencerPageStart();
  uint64_t patternSignature = neokeyBuildPatternSignature();
  bool stateChanged =
    track != neokeyLastTrack ||
    page != neokeyLastPage ||
    playStep != neokeyLastPlayStep ||
    running != neokeyLastSequencerRunning ||
    patternSignature != neokeyLastPatternSignature;

  if (!stateChanged) return;
  if (now - neokeyLastRefreshMs < NEOKEY_REFRESH_MS) return;

  if (!neokeySelectPcaChannel()) {
    neokeyRestoreDisplayBusClock();
    return;
  }

  uint8_t changedBoards = 0;

  for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
    int absoluteStep = pageStart + i;
    bool inLength = absoluteStep < getSequencerLength();
    bool active = inLength && isSequencerTrackStepEnabled(track, absoluteStep);
    bool playhead = inLength && running && absoluteStep == playStep;
    uint32_t color = inLength
      ? neokeyStepColor(track, active, playhead)
      : neokeyColor(0, 0, 0);

    if (!neokeyStepColorCacheValid || color != neokeyLastStepColors[i]) {
      neokeySetStepPixel(i, color);
      neokeyLastStepColors[i] = color;
      changedBoards |= (uint8_t)1U << (i / NEOKEY_KEYS_PER_BOARD);
    }
  }

  // setPixelColor() writes to seesaw immediately, so avoid 16 I2C writes on
  // every playhead movement and latch only boards whose pixels changed.
  for (int board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    if ((changedBoards & ((uint8_t)1U << board)) && neokeyBoardReady[board]) {
      neokeyBoards[board].pixels.show();
    }
  }
  neokeyRestoreDisplayBusClock();

  if (isSerialDiagnosticsEnabled() && running) {
    Serial.print("[SEQ][LED] ms=");
    Serial.print(now);
    Serial.print(" visual=");
    Serial.print(playStep + 1);
    Serial.print(" localLed=");
    Serial.print(playStep - pageStart + 1);
    Serial.print(" page=");
    Serial.print(page + 1);
    Serial.print(" boards=0x");
    Serial.println(changedBoards, HEX);
  }

  neokeyLastRefreshMs = now;
  neokeyLastTrack = track;
  neokeyLastPlayStep = playStep;
  neokeyLastPage = page;
  neokeyLastSequencerRunning = running;
  neokeyLastPatternSignature = patternSignature;
  neokeyStepColorCacheValid = true;
}

void initNeoKeySequencerPanel() {
  Wire.begin(NEOKEY_I2C_SDA, NEOKEY_I2C_SCL);
  Wire.setClock(NEOKEY_I2C_CLOCK_HZ);
  neokeyAnyReady = false;
  delay(180);

  for (uint8_t attempt = 0; attempt < NEOKEY_BOOT_RETRY_COUNT; attempt++) {
    for (uint8_t board = 0; board < NEOKEY_BOARD_COUNT; board++) {
      if (!neokeyBoardReady[board]) {
        neokeyBoardReady[board] = neokeyBeginBoard(board);
      }
    }

    neokeyRecomputeReadyState();
    bool allReady = true;
    for (uint8_t board = 0; board < NEOKEY_BOARD_COUNT; board++) {
      allReady = allReady && neokeyBoardReady[board];
    }
    if (allReady) break;
    delay(120);
  }

  if (isSerialDiagnosticsEnabled()) {
    serialDebugBegin("NEOKEY INITIALIZATION");
    for (uint8_t board = 0; board < NEOKEY_BOARD_COUNT; board++) {
      Serial.print("NeoKey PCA channel 4 ");
      Serial.print(neokeyBoardReady[board] ? "found at 0x" : "missing at 0x");
      Serial.println(NEOKEY_BOARD_ADDRESSES[board], HEX);
    }
    serialDebugEnd("NEOKEY INITIALIZATION");
  }

  neokeyRestoreDisplayBusClock();

  if (neokeyAnyReady) {
    for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
      neokeySetStepPixel(i, neokeyColor(24, 24, 24));
      neokeyShowAll();
      delay(45);
    }

    neokeyClearAll();
  }

  neokeyLastReconnectMs = millis();
  neokeySyncButtonDebounce(neokeyAnyReady ? neokeyReadButtonsRaw() : 0x00, millis());
}
