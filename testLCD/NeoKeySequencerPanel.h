#pragma once

#include <Wire.h>
#include "Adafruit_NeoKey_1x4.h"
#include "seesaw_neopixel.h"

// 2x Adafruit NeoKey 1x4 QT over STEMMA QT.
// Board 0 controls steps 1-4, board 1 controls steps 5-8.

const uint8_t NEOKEY_BOARD_COUNT = 2;
const uint8_t NEOKEY_KEYS_PER_BOARD = 4;
const uint8_t NEOKEY_TOTAL_KEYS = NEOKEY_BOARD_COUNT * NEOKEY_KEYS_PER_BOARD;
const uint8_t NEOKEY_ADDR_A = 0x30;
const uint8_t NEOKEY_ADDR_B = 0x31;
const uint8_t NEOKEY_LOCAL_KEY_MAP[NEOKEY_KEYS_PER_BOARD] = { 3, 2, 1, 0 };
const uint32_t NEOKEY_KEY_POLL_MS = 10;
const uint32_t NEOKEY_KEY_DEBOUNCE_MS = 30;
const uint32_t NEOKEY_REFRESH_MS = 25;
const uint8_t NEOKEY_BRIGHTNESS = 36;

Adafruit_NeoKey_1x4 neokeyBoards[NEOKEY_BOARD_COUNT] = {
  Adafruit_NeoKey_1x4(NEOKEY_ADDR_A),
  Adafruit_NeoKey_1x4(NEOKEY_ADDR_B),
};

bool neokeyBoardReady[NEOKEY_BOARD_COUNT] = { false, false };
bool neokeyAnyReady = false;
uint8_t neokeyLastRawKeys = 0;
uint8_t neokeyStableKeys = 0;
uint32_t neokeyLastPollMs = 0;
uint32_t neokeyLastRefreshMs = 0;
bool neokeyButtonLastRaw[NEOKEY_TOTAL_KEYS] = { false };
bool neokeyButtonStable[NEOKEY_TOTAL_KEYS] = { false };
uint32_t neokeyButtonChangedAtMs[NEOKEY_TOTAL_KEYS] = { 0 };
int neokeyLastVoice = -1;
int neokeyLastStep = -1;
int neokeyLastPlayStep = -1;
int neokeyLastLedMask = -1;
bool neokeyLastSequencerRunning = false;

bool neokeySequencerControlActive() {
  return uiMode != MODE_BOOT && uiMode != MODE_PREMENU && uiMode != MODE_ABOUT;
}

bool neokeySequencerRunning() {
  return transportRunning && sequencerEnabled;
}

uint32_t neokeyColor(uint8_t r, uint8_t g, uint8_t b) {
  return seesaw_NeoPixel::Color(r, g, b);
}

uint32_t neokeyStepColor(int stepIndex, bool selected, bool activePattern, bool playhead) {
  (void)stepIndex;

  (void)selected;

  if (playhead) return neokeyColor(255, 255, 255);
  if (activePattern) return neokeyColor(180, 0, 0);
  return 0;
}

void neokeySetStepPixel(int stepIndex, uint32_t color) {
  if (stepIndex < 0 || stepIndex >= NEOKEY_TOTAL_KEYS) return;

  int board = stepIndex / NEOKEY_KEYS_PER_BOARD;
  int localKey = NEOKEY_LOCAL_KEY_MAP[stepIndex % NEOKEY_KEYS_PER_BOARD];

  if (!neokeyBoardReady[board]) return;

  neokeyBoards[board].pixels.setPixelColor(localKey, color);
}

void neokeyShowAll() {
  for (int board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    if (neokeyBoardReady[board]) {
      neokeyBoards[board].pixels.show();
    }
  }
}

void neokeyClearAll() {
  for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
    neokeySetStepPixel(i, 0);
  }

  neokeyShowAll();
}

void neokeyMarkSequencerDirty(bool retriggerAudio) {
  valueDirty = true;
  fullRedraw = false;

  if (retriggerAudio) {
    synthRetriggerRequested = true;
  }
}

void neokeySelectStep(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return;

  sequencerSelectedStep = stepIndex;

  if (!sequencerEnabled && !isMixerLayerEnabled()) {
    sequencerCurrentStep = sequencerSelectedStep;
    resetSequencerClock();
  }
}

void neokeyToggleStep(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return;

  int voice = getActiveSequencerVoice();
  neokeySelectStep(stepIndex);
  sequencerStepEnabled[voice][stepIndex] = !sequencerStepEnabled[voice][stepIndex];
  neokeyMarkSequencerDirty(false);
}

uint8_t neokeyReadButtonsRaw() {
  uint8_t rawKeys = 0;

  for (int board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    if (!neokeyBoardReady[board]) continue;

    uint8_t boardKeys = neokeyBoards[board].read() & 0x0F;
    uint8_t mappedKeys = 0;

    for (int i = 0; i < NEOKEY_KEYS_PER_BOARD; i++) {
      if (boardKeys & (1 << NEOKEY_LOCAL_KEY_MAP[i])) {
        mappedKeys |= (1 << i);
      }
    }

    rawKeys |= (uint8_t)(mappedKeys << (board * NEOKEY_KEYS_PER_BOARD));
  }

  return rawKeys;
}

void neokeySyncButtonDebounce(uint8_t rawKeys, uint32_t now) {
  neokeyLastRawKeys = rawKeys;
  neokeyStableKeys = rawKeys;

  for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
    bool raw = (rawKeys & (1 << i)) != 0;
    neokeyButtonLastRaw[i] = raw;
    neokeyButtonStable[i] = raw;
    neokeyButtonChangedAtMs[i] = now;
  }
}

void pollNeoKeySequencerKeys() {
  if (!neokeyAnyReady) return;

  uint32_t now = millis();
  if (now - neokeyLastPollMs < NEOKEY_KEY_POLL_MS) return;
  neokeyLastPollMs = now;

  uint8_t rawKeys = neokeyReadButtonsRaw();
  neokeyLastRawKeys = rawKeys;

  if (!neokeySequencerControlActive()) {
    neokeySyncButtonDebounce(rawKeys, now);
    return;
  }

  uint8_t stableMask = 0;
  uint8_t pressed = 0;

  for (int i = 0; i < NEOKEY_TOTAL_KEYS; i++) {
    bool raw = (rawKeys & (1 << i)) != 0;

    if (raw != neokeyButtonLastRaw[i]) {
      neokeyButtonLastRaw[i] = raw;
      neokeyButtonChangedAtMs[i] = now;
    }

    if (now - neokeyButtonChangedAtMs[i] >= NEOKEY_KEY_DEBOUNCE_MS &&
        raw != neokeyButtonStable[i]) {
      neokeyButtonStable[i] = raw;

      if (raw) {
        pressed |= (1 << i);
      }
    }

    if (neokeyButtonStable[i]) {
      stableMask |= (1 << i);
    }
  }

  neokeyStableKeys = stableMask;

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    if (pressed & (1 << i)) {
      neokeyToggleStep(i);
    }
  }
}

uint8_t neokeyBuildStepLedMask() {
  if (neokeySequencerRunning()) {
    int step = clampInt((int)sequencerCurrentStep, 0, getSequencerLength() - 1);
    return (uint8_t)(1 << step);
  }

  int voice = getActiveSequencerVoice();
  uint8_t mask = 0;

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    if (sequencerStepEnabled[voice][i]) {
      mask |= (1 << i);
    }
  }

  return mask;
}

void refreshNeoKeySequencerPanel() {
  if (!neokeyAnyReady) return;

  uint32_t now = millis();
  int voice = getActiveSequencerVoice();
  int step = clampInt(sequencerSelectedStep, 0, SEQUENCER_STEP_COUNT - 1);
  int playStep = clampInt((int)sequencerCurrentStep, 0, getSequencerLength() - 1);
  bool running = neokeySequencerRunning();
  uint8_t ledMask = neokeyBuildStepLedMask();
  bool stateChanged =
    voice != neokeyLastVoice ||
    step != neokeyLastStep ||
    playStep != neokeyLastPlayStep ||
    running != neokeyLastSequencerRunning ||
    ledMask != neokeyLastLedMask;

  if (!stateChanged && now - neokeyLastRefreshMs < NEOKEY_REFRESH_MS) return;

  neokeyLastRefreshMs = now;
  neokeyLastVoice = voice;
  neokeyLastStep = step;
  neokeyLastPlayStep = playStep;
  neokeyLastSequencerRunning = running;
  neokeyLastLedMask = ledMask;

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    bool activePattern = isSequencerStepEnabled(voice, i);
    bool selected = neokeySequencerControlActive() && i == step;
    bool playhead = running && (ledMask & (1 << i));

    neokeySetStepPixel(i, neokeyStepColor(i, selected, activePattern, playhead));
  }

  neokeyShowAll();
}

void initNeoKeySequencerPanel() {
  Wire.begin(NEOKEY_I2C_SDA, NEOKEY_I2C_SCL);
  Wire.setClock(400000);

  for (int board = 0; board < NEOKEY_BOARD_COUNT; board++) {
    uint8_t address = board == 0 ? NEOKEY_ADDR_A : NEOKEY_ADDR_B;
    neokeyBoardReady[board] = neokeyBoards[board].begin(address);

    if (neokeyBoardReady[board]) {
      neokeyBoards[board].pixels.setBrightness(NEOKEY_BRIGHTNESS);
      neokeyBoards[board].pixels.clear();
      neokeyBoards[board].pixels.show();
      neokeyAnyReady = true;

      Serial.print("NeoKey 1x4 found at 0x");
      Serial.println(address, HEX);
    } else {
      Serial.print("NeoKey 1x4 missing at 0x");
      Serial.println(address, HEX);
    }
  }

  if (neokeyAnyReady) {
    for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
      neokeySetStepPixel(i, neokeyColor(24, 24, 24));
      neokeyShowAll();
      delay(45);
    }

    neokeyClearAll();
  }

  neokeySyncButtonDebounce(0x00, millis());
}
