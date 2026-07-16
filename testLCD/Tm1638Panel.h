#pragma once

#include <TM1638plus.h>

// LED&KEY TM1638 step strip.
// S1-S8 control only sequencer steps 1-8 for the selected voice track. The LCD
// + encoder keep voice selection, notes, shape, utility, BPM, gate, direction,
// and transport duties.

const uint8_t TM1638_BRIGHTNESS = 2;
const uint32_t TM1638_KEY_POLL_MS = 15;
const uint32_t TM1638_KEY_DEBOUNCE_MS = 35;
const uint32_t TM1638_REFRESH_MS = 60;
const uint8_t TM1638_READ_DELAY_US = 20;
const uint8_t TM1638_WRITE_DELAY_US = 20;
const uint8_t TM1638_BUTTONS_MODE = 0x42;
const uint8_t TM1638_WRITE_FIXED_ADDRESS = 0x44;
const uint8_t TM1638_DATA_BASE_ADDRESS = 0xC0;
const uint8_t TM1638_LED_BASE_ADDRESS = 0xC1;
const uint8_t TM1638_LED_ON_VALUE = 0xFF;
const uint8_t TM1638_LED_OFF_VALUE = 0x00;
const uint16_t TM1638_LED_ALL_TEST_MS = 700;
const uint16_t TM1638_LED_SCAN_TEST_MS = 700;
const uint16_t TM1638_ADDRESS_SCAN_TEST_MS = 900;
const uint8_t TM1638_LED_POSITION_MAP[SEQUENCER_STEP_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7 };
const bool TM1638_USE_DIO_OUTPUT_ENABLE = true;

TM1638plus tm1638Module(TM1638_STB, TM1638_CLK, TM1638_DIO, true);

uint8_t tm1638LastRawKeys = 0;
uint8_t tm1638StableKeys = 0;
uint8_t tm1638LiveKeys = 0;
uint32_t tm1638LastPollMs = 0;
uint32_t tm1638LastRefreshMs = 0;
uint32_t tm1638PressCount[SEQUENCER_STEP_COUNT] = { 0 };
bool tm1638ButtonLastRaw[SEQUENCER_STEP_COUNT] = { false };
bool tm1638ButtonStable[SEQUENCER_STEP_COUNT] = { false };
uint32_t tm1638ButtonChangedAtMs[SEQUENCER_STEP_COUNT] = { 0 };
int tm1638LastVoice = -1;
int tm1638LastStep = -1;
int tm1638LastPlayStep = -1;
int tm1638LastBpm = -1;
int tm1638LastLedMask = -1;
bool tm1638LastSequencerRunning = false;
bool tm1638Ready = false;

bool tm1638SequencerControlActive() {
  return uiMode != MODE_BOOT && uiMode != MODE_PREMENU && uiMode != MODE_ABOUT;
}

bool tm1638SequencerRunning() {
  return transportRunning && sequencerEnabled;
}

void tm1638SetDioOutputEnabled(bool enabled) {
  if (!TM1638_USE_DIO_OUTPUT_ENABLE) return;

  digitalWrite(TM1638_DIO_OE, enabled ? LOW : HIGH);
}

void tm1638ShiftOutByte(uint8_t value) {
  tm1638SetDioOutputEnabled(true);
  pinMode(TM1638_DIO, OUTPUT);

  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(TM1638_DIO, (value & (1 << i)) != 0);
    digitalWrite(TM1638_CLK, HIGH);
    delayMicroseconds(TM1638_WRITE_DELAY_US);
    digitalWrite(TM1638_CLK, LOW);
    delayMicroseconds(TM1638_WRITE_DELAY_US);
  }
}

uint8_t tm1638ShiftInByte() {
  uint8_t value = 0;
  tm1638SetDioOutputEnabled(false);
  pinMode(TM1638_DIO, INPUT_PULLUP);

  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(TM1638_CLK, HIGH);
    delayMicroseconds(TM1638_READ_DELAY_US);

    if (digitalRead(TM1638_DIO)) {
      value |= (1 << i);
    }

    digitalWrite(TM1638_CLK, LOW);
    delayMicroseconds(TM1638_READ_DELAY_US);
  }

  return value;
}

uint8_t tm1638ReadButtonsRaw() {
  uint8_t buttons = 0;

  digitalWrite(TM1638_CLK, LOW);
  digitalWrite(TM1638_STB, LOW);
  tm1638ShiftOutByte(TM1638_BUTTONS_MODE);

  for (uint8_t i = 0; i < 4; i++) {
    buttons |= tm1638ShiftInByte() << i;
  }

  pinMode(TM1638_DIO, OUTPUT);
  tm1638SetDioOutputEnabled(true);
  digitalWrite(TM1638_STB, HIGH);

  return buttons;
}

void tm1638SendCommand(uint8_t command) {
  digitalWrite(TM1638_CLK, LOW);
  digitalWrite(TM1638_STB, LOW);
  tm1638ShiftOutByte(command);
  digitalWrite(TM1638_STB, HIGH);
}

void tm1638WriteData(uint8_t address, uint8_t value) {
  tm1638SendCommand(TM1638_WRITE_FIXED_ADDRESS);
  digitalWrite(TM1638_CLK, LOW);
  digitalWrite(TM1638_STB, LOW);
  tm1638ShiftOutByte(address);
  tm1638ShiftOutByte(value);
  digitalWrite(TM1638_STB, HIGH);
}

void tm1638WriteLed(int position, bool enabled) {
  if (position < 0 || position >= SEQUENCER_STEP_COUNT) return;

  uint8_t mappedPosition = TM1638_LED_POSITION_MAP[position];
  if (mappedPosition >= SEQUENCER_STEP_COUNT) return;

  uint8_t address = TM1638_LED_BASE_ADDRESS + (mappedPosition << 1);
  tm1638WriteData(address, enabled ? TM1638_LED_ON_VALUE : TM1638_LED_OFF_VALUE);
}

void tm1638ClearDataAddresses() {
  for (uint8_t i = 0; i < 16; i++) {
    tm1638WriteData(TM1638_DATA_BASE_ADDRESS + i, 0x00);
  }
}

void tm1638DisplayPadded(const char* text) {
  char padded[9];
  int i = 0;

  for (; i < 8 && text[i] != '\0'; i++) {
    padded[i] = text[i];
  }

  for (; i < 8; i++) {
    padded[i] = ' ';
  }

  padded[8] = '\0';
  tm1638Module.displayText(padded);
}

void tm1638MarkSequencerDirty(bool retriggerAudio) {
  valueDirty = true;
  fullRedraw = false;

  if (retriggerAudio) {
    synthRetriggerRequested = true;
  }
}

void tm1638SelectStep(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return;

  sequencerSelectedStep = stepIndex;

  if (!sequencerEnabled && !isMixerLayerEnabled()) {
    sequencerCurrentStep = sequencerSelectedStep;
    resetSequencerClock();
  }
}

void tm1638ToggleStep(int stepIndex) {
  if (stepIndex < 0 || stepIndex >= SEQUENCER_STEP_COUNT) return;

  int voice = getActiveSequencerVoice();
  tm1638PressCount[stepIndex]++;
  tm1638SelectStep(stepIndex);
  sequencerStepEnabled[voice][stepIndex] = !sequencerStepEnabled[voice][stepIndex];
  tm1638MarkSequencerDirty(false);
}

void tm1638SyncButtonDebounce(uint8_t rawKeys, uint32_t now) {
  tm1638LastRawKeys = rawKeys;
  tm1638StableKeys = rawKeys;
  tm1638LiveKeys = rawKeys;

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    bool raw = (rawKeys & (1 << i)) != 0;
    tm1638ButtonLastRaw[i] = raw;
    tm1638ButtonStable[i] = raw;
    tm1638ButtonChangedAtMs[i] = now;
  }
}

void pollTm1638Keys() {
  if (!tm1638Ready) return;

  uint32_t now = millis();
  if (now - tm1638LastPollMs < TM1638_KEY_POLL_MS) return;
  tm1638LastPollMs = now;

  uint8_t rawKeys = tm1638ReadButtonsRaw();
  tm1638LastRawKeys = rawKeys;
  tm1638LiveKeys = rawKeys;

  if (!tm1638SequencerControlActive()) {
    tm1638SyncButtonDebounce(rawKeys, now);
    return;
  }

  uint8_t stableMask = 0;
  uint8_t pressed = 0;

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    bool raw = (rawKeys & (1 << i)) != 0;

    if (raw != tm1638ButtonLastRaw[i]) {
      tm1638ButtonLastRaw[i] = raw;
      tm1638ButtonChangedAtMs[i] = now;
    }

    if (now - tm1638ButtonChangedAtMs[i] >= TM1638_KEY_DEBOUNCE_MS &&
        raw != tm1638ButtonStable[i]) {
      tm1638ButtonStable[i] = raw;

      if (raw) {
        pressed |= (1 << i);
      }
    }

    if (tm1638ButtonStable[i]) {
      stableMask |= (1 << i);
    }
  }

  tm1638StableKeys = stableMask;

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    if (pressed & (1 << i)) {
      tm1638ToggleStep(i);
    }
  }
}

uint8_t tm1638BuildStepLedMask() {
  if (tm1638SequencerRunning()) {
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

void tm1638ApplyStepLeds(uint8_t ledMask) {
  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    tm1638WriteLed(i, (ledMask & (1 << i)) != 0);
  }
}

void tm1638RunLedSelfTest() {
  tm1638DisplayPadded("LEDTEST");
  tm1638ApplyStepLeds(0xFF);
  delay(TM1638_LED_ALL_TEST_MS);

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    char label[9];
    snprintf(label, sizeof(label), "LED %d", i + 1);
    tm1638DisplayPadded(label);
    tm1638ApplyStepLeds((uint8_t)(1 << i));
    delay(TM1638_LED_SCAN_TEST_MS);
  }

  tm1638ApplyStepLeds(0x00);
}

void tm1638RunAddressSelfTest() {
  for (uint8_t addressOffset = 0; addressOffset < 16; addressOffset++) {
    uint8_t address = TM1638_DATA_BASE_ADDRESS + addressOffset;
    char label[9];

    tm1638ClearDataAddresses();
    snprintf(label, sizeof(label), "ADR %02X", address);
    tm1638DisplayPadded(label);
    tm1638WriteData(address, 0xFF);
    delay(TM1638_ADDRESS_SCAN_TEST_MS);
  }

  tm1638ClearDataAddresses();
}

void tm1638BuildStepText(char* buffer, size_t size) {
  int voice = getActiveSequencerVoice();
  int step = clampInt(sequencerSelectedStep, 0, SEQUENCER_STEP_COUNT - 1);

  if (sequencerStepEnabled[voice][step]) {
    snprintf(buffer, size, "V%dS%d%+03d", voice + 1, step + 1, sequencerStepNotes[voice][step]);
  } else {
    snprintf(buffer, size, "V%dS%dRST", voice + 1, step + 1);
  }
}

void tm1638BuildBpmText(char* buffer, size_t size) {
  snprintf(buffer, size, "%d BPM", sequencerBpm);
}

void refreshTm1638Panel() {
  if (!tm1638Ready) return;

  uint32_t now = millis();
  int voice = getActiveSequencerVoice();
  int step = clampInt(sequencerSelectedStep, 0, SEQUENCER_STEP_COUNT - 1);
  int playStep = clampInt((int)sequencerCurrentStep, 0, getSequencerLength() - 1);
  bool sequencerRunning = tm1638SequencerRunning();
  uint8_t ledMask = tm1638BuildStepLedMask();
  bool stateChanged =
    voice != tm1638LastVoice ||
    step != tm1638LastStep ||
    playStep != tm1638LastPlayStep ||
    sequencerBpm != tm1638LastBpm ||
    sequencerRunning != tm1638LastSequencerRunning ||
    ledMask != tm1638LastLedMask;

  if (!stateChanged && now - tm1638LastRefreshMs < TM1638_REFRESH_MS) return;
  tm1638LastRefreshMs = now;
  tm1638LastVoice = voice;
  tm1638LastStep = step;
  tm1638LastPlayStep = playStep;
  tm1638LastBpm = sequencerBpm;
  tm1638LastSequencerRunning = sequencerRunning;
  tm1638LastLedMask = ledMask;

  if (sequencerRunning) {
    char text[16];
    tm1638BuildBpmText(text, sizeof(text));
    tm1638DisplayPadded(text);
    tm1638ApplyStepLeds(ledMask);
    return;
  }

  if (!tm1638SequencerControlActive()) {
    tm1638DisplayPadded("STEPSEQ");
    tm1638ApplyStepLeds(ledMask);
    return;
  }

  char text[16];
  tm1638BuildStepText(text, sizeof(text));
  tm1638DisplayPadded(text);
  tm1638ApplyStepLeds(ledMask);
}

void initTm1638Panel() {
  pinMode(TM1638_DIO_OE, OUTPUT);
  tm1638SetDioOutputEnabled(true);
  tm1638Module.setHFIN_DELAY(TM1638_READ_DELAY_US);
  tm1638Module.setHFOUT_DELAY(TM1638_WRITE_DELAY_US);
  tm1638Module.displayBegin();
  tm1638Module.brightness(TM1638_BRIGHTNESS);
  tm1638RunLedSelfTest();
  tm1638RunAddressSelfTest();
  tm1638DisplayPadded("STEPSEQ");
  tm1638SyncButtonDebounce(0x00, millis());
  tm1638Ready = true;
}
