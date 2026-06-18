#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include <math.h>
#include <string.h>

// =======================
// PIN DISPLAY
// =======================

#define TFT_CS    5
#define TFT_DC    4
#define TFT_RST   2
#define TFT_SCK   18
#define TFT_MOSI  23

// =======================
// PIN ENCODER / BUTTON
// =======================

#define ENC_A     32
#define ENC_B     33
#define ENC_SW    25
#define BTN_K0    26

// =======================
// PIN I2S / PCM5102
// =======================

#define PCM5102_BCK   27
#define PCM5102_LRCK  14
#define PCM5102_DIN   22

// =======================
// DISPLAY
// =======================

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
I2SStream btI2s;
BluetoothA2DPSink a2dpSink(btI2s);

const int SCREEN_W = 160;
const int SCREEN_H = 128;

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

const uint16_t COL_BG = rgb565(5, 7, 14);
const uint16_t COL_PANEL = rgb565(15, 19, 31);
const uint16_t COL_HEADER = rgb565(21, 25, 40);
const uint16_t COL_LINE = rgb565(42, 47, 62);
const uint16_t COL_MUTED = rgb565(118, 126, 142);
const uint16_t COL_INK = rgb565(238, 235, 220);
const uint16_t COL_NOTE = rgb565(59, 220, 181);
const uint16_t COL_GOLD = rgb565(246, 191, 65);
const uint16_t COL_CORAL = rgb565(255, 91, 76);
const uint16_t COL_VIOLET = rgb565(137, 103, 255);
const uint16_t COL_SKY = rgb565(98, 178, 255);

const uint16_t ITEM_COLORS[] = {
  COL_NOTE,
  COL_GOLD,
  COL_CORAL,
  COL_VIOLET,
  COL_SKY,
  COL_INK
};

const int ITEM_COLOR_COUNT = sizeof(ITEM_COLORS) / sizeof(ITEM_COLORS[0]);

const float TWO_PI_F = 6.2831853f;
const int SPECTRUM_MAX_BARS = 32;
const float SPECTRUM_MIN_HZ = 20.0f;

const int FFT_RANGE_VALUES[] = { 1000, 4000, 8000, 12000 };
const char* FFT_RANGE_LABELS[] = { "1K", "4K", "8K", "12K" };
const int FFT_RANGE_COUNT = sizeof(FFT_RANGE_VALUES) / sizeof(FFT_RANGE_VALUES[0]);

const int FFT_BIN_VALUES[] = { 12, 18, 24, 32 };
const int FFT_BIN_COUNT = sizeof(FFT_BIN_VALUES) / sizeof(FFT_BIN_VALUES[0]);

const char* FFT_WINDOW_NAMES[] = { "HANN", "RECT", "BLACK", "FLAT" };
const int FFT_WINDOW_COUNT = sizeof(FFT_WINDOW_NAMES) / sizeof(FFT_WINDOW_NAMES[0]);

const char* BT_AUDIO_NAME = "Polyrhythmic Glider";
const uint32_t BT_STREAM_TIMEOUT_MS = 1400;
const uint32_t BT_UI_REFRESH_MS = 250;
const uint32_t BT_RSSI_REFRESH_MS = 1800;
const uint32_t BT_HEADER_REFRESH_MS = 350;

// =======================
// ENCODER ISR
// =======================

portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

volatile int32_t encoderRawTicks = 0;
volatile uint8_t encoderLastState = 0;

const int8_t encoderTable[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void IRAM_ATTR encoderISR() {
  uint8_t a = digitalRead(ENC_A);
  uint8_t b = digitalRead(ENC_B);

  uint8_t newState = (a << 1) | b;
  uint8_t index = (encoderLastState << 2) | newState;

  int8_t movement = encoderTable[index];
  encoderLastState = newState;

  if (movement == 0) return;

  portENTER_CRITICAL_ISR(&encoderMux);
  encoderRawTicks += movement;
  portEXIT_CRITICAL_ISR(&encoderMux);
}

int32_t consumeEncoderTicks() {
  int32_t ticks = 0;

  portENTER_CRITICAL(&encoderMux);
  ticks = encoderRawTicks;
  encoderRawTicks = 0;
  portEXIT_CRITICAL(&encoderMux);

  return ticks;
}

// =======================
// INPUT
// =======================

const int ENCODER_STEPS_PER_DETENT = 2;
int32_t encoderSubSteps = 0;

const uint32_t debounceMs = 35;

bool lastPushState = HIGH;
bool lastK0State = HIGH;

uint32_t lastPushDebounceMs = 0;
uint32_t lastK0DebounceMs = 0;

bool pushEvent = false;
bool k0Event = false;

int32_t encoderUiStepsTotal = 0;
uint32_t pushPressCount = 0;
uint32_t k0PressCount = 0;

int32_t getEncoderSteps() {
  int32_t ticks = consumeEncoderTicks();

  if (ticks == 0) return 0;

  encoderSubSteps += ticks;

  int32_t steps = 0;

  while (encoderSubSteps >= ENCODER_STEPS_PER_DETENT) {
    steps++;
    encoderSubSteps -= ENCODER_STEPS_PER_DETENT;
  }

  while (encoderSubSteps <= -ENCODER_STEPS_PER_DETENT) {
    steps--;
    encoderSubSteps += ENCODER_STEPS_PER_DETENT;
  }

  encoderUiStepsTotal += steps;

  return steps;
}

void updateButtons() {
  uint32_t now = millis();

  bool pushNow = digitalRead(ENC_SW);
  bool k0Now = digitalRead(BTN_K0);

  if (pushNow != lastPushState && now - lastPushDebounceMs > debounceMs) {
    lastPushDebounceMs = now;
    lastPushState = pushNow;

    if (pushNow == LOW) {
      pushEvent = true;
      pushPressCount++;
    }
  }

  if (k0Now != lastK0State && now - lastK0DebounceMs > debounceMs) {
    lastK0DebounceMs = now;
    lastK0State = k0Now;

    if (k0Now == LOW) {
      k0Event = true;
      k0PressCount++;
    }
  }
}

// =======================
// MENU DATA
// =======================

enum UiMode {
  MODE_BOOT,
  MODE_PREMENU,
  MODE_MENU,
  MODE_EDIT,
  MODE_ABOUT
};

void setMode(UiMode newMode);

UiMode uiMode = MODE_BOOT;
UiMode lastDrawnMode = MODE_ABOUT;

const char* waveformNames[] = {
  "SINE",
  "SQUARE",
  "TRI",
  "SAW"
};

int waveform = 0;
int frequency = 220;
int volume = 80;
int attack = 10;
int decay = 80;
int waveFold = 12;
int wavePhase = 0;
int fftRangeIndex = 1;
int fftWindow = 0;
int fftSmoothing = 35;
int fftBinsIndex = 1;
bool fftPeakHold = true;
bool btAudioStarted = false;
uint32_t btStartedAtMs = 0;
volatile uint32_t btLastAudioMs = 0;
volatile uint32_t btPacketCount = 0;

enum MenuAction {
  ACTION_WAVEFORM,
  ACTION_PITCH,
  ACTION_LEVEL,
  ACTION_WAVE_FOLD,
  ACTION_WAVE_PHASE,
  ACTION_ATTACK,
  ACTION_DECAY,
  ACTION_SPECTRUM,
  ACTION_FFT_RANGE,
  ACTION_FFT_WINDOW,
  ACTION_FFT_SMOOTH,
  ACTION_FFT_HOLD,
  ACTION_FFT_BINS,
  ACTION_SCOPE,
  ACTION_BT_RECEIVER,
  ACTION_BT_DEVICE,
  ACTION_BT_I2S,
  ACTION_BT_OUTPUT,
  ACTION_INPUTS,
  ACTION_DISPLAY_TEST,
  ACTION_ABOUT
};

MenuAction getSelectedAction();

struct MenuItem {
  const char* tag;
  const char* label;
  MenuAction action;
};

MenuItem menuItems[] = {
  { "OSC", "Shape", ACTION_WAVEFORM },
  { "TUNE", "Pitch", ACTION_PITCH },
  { "GAIN", "Level", ACTION_LEVEL },
  { "FOLD", "Wavefold", ACTION_WAVE_FOLD },
  { "PHAS", "Phase", ACTION_WAVE_PHASE },
  { "RISE", "Attack", ACTION_ATTACK },
  { "FALL", "Decay", ACTION_DECAY },
  { "SCP", "Wave Scope", ACTION_SCOPE },
  { "SPEC", "Analyzer", ACTION_SPECTRUM },
  { "RNG", "FFT Range", ACTION_FFT_RANGE },
  { "WIN", "Window", ACTION_FFT_WINDOW },
  { "AVG", "Smoothing", ACTION_FFT_SMOOTH },
  { "HOLD", "Peak Hold", ACTION_FFT_HOLD },
  { "BIN", "FFT Bins", ACTION_FFT_BINS },
  { "IN", "Inputs", ACTION_INPUTS },
  { "LCD", "Display", ACTION_DISPLAY_TEST },
  { "INFO", "Stage", ACTION_ABOUT },
  { "BT", "Receiver", ACTION_BT_RECEIVER },
  { "DEV", "Pair Name", ACTION_BT_DEVICE },
  { "I2S", "PCM5102 Bus", ACTION_BT_I2S },
  { "AUX", "Line Output", ACTION_BT_OUTPUT }
};

const uint8_t MENU_WAVE_ITEMS[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
const uint8_t MENU_FFT_ITEMS[] = { 8, 9, 10, 11, 12, 13 };
const uint8_t MENU_UTILITY_ITEMS[] = { 2, 14, 15, 16 };
const uint8_t MENU_TEST_ITEMS[] = { 15, 14, 7, 8, 16 };
const uint8_t MENU_BT_ITEMS[] = { 17, 18, 19, 20 };

struct AreaChoice {
  const char* tag;
  const char* label;
  const uint8_t* items;
  int itemCount;
};

AreaChoice areaChoices[] = {
  { "WAVE", "Wave Engine", MENU_WAVE_ITEMS, sizeof(MENU_WAVE_ITEMS) / sizeof(MENU_WAVE_ITEMS[0]) },
  { "FFT", "Spectrum Lab", MENU_FFT_ITEMS, sizeof(MENU_FFT_ITEMS) / sizeof(MENU_FFT_ITEMS[0]) },
  { "BT", "Bluetooth RX", MENU_BT_ITEMS, sizeof(MENU_BT_ITEMS) / sizeof(MENU_BT_ITEMS[0]) },
  { "UTIL", "Utility", MENU_UTILITY_ITEMS, sizeof(MENU_UTILITY_ITEMS) / sizeof(MENU_UTILITY_ITEMS[0]) },
  { "TEST", "Hardware Test", MENU_TEST_ITEMS, sizeof(MENU_TEST_ITEMS) / sizeof(MENU_TEST_ITEMS[0]) }
};

const int TYPE_COUNT = sizeof(areaChoices) / sizeof(areaChoices[0]);

int selectedType = 0;
int activeType = 0;
int lastSelectedType = -1;

int selectedItem = 0;
int lastSelectedItem = -1;

int topItem = 0;
int lastTopItem = -1;

const int VISIBLE_ROWS = 5;
const uint32_t menuStepCooldownMs = 80;
uint32_t lastMenuMoveMs = 0;

bool fullRedraw = true;
bool valueDirty = true;

int lastWaveform = -1;
int lastFrequency = -1;
int lastVolume = -1;
int lastAttack = -1;
int lastDecay = -1;
int lastWaveFold = -1;
int lastWavePhase = -1;
int lastFftRangeIndex = -1;
int lastFftWindow = -1;
int lastFftSmoothing = -1;
int lastFftBinsIndex = -1;
int lastFftPeakHold = -1;
bool lastBtStreaming = false;
uint32_t lastBtPacketDrawn = 0;
uint32_t lastBtUiRefreshMs = 0;

uint32_t lastBtHeaderRefreshMs = 0;
uint32_t lastBtRssiRequestMs = 0;
int8_t btRssiDelta = 0;
bool btRssiValid = false;

const uint32_t bootDurationMs = 3600;
uint32_t bootStartMs = 0;
int lastBootProgress = -1;

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

void btAudioDataReceived() {
  btLastAudioMs = millis();
  btPacketCount++;
}

void btAudioStreamReceived(const uint8_t* data, uint32_t length) {
  (void)data;
  (void)length;

  btAudioDataReceived();
}

void btRssiReceived(esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi) {
  btRssiDelta = rssi.rssi_delta;
  btRssiValid = true;
}

void startBluetoothAudio() {
  if (btAudioStarted) return;

  auto cfg = btI2s.defaultConfig();
  cfg.pin_bck = PCM5102_BCK;
  cfg.pin_ws = PCM5102_LRCK;
  cfg.pin_data = PCM5102_DIN;
  btI2s.begin(cfg);

  a2dpSink.set_stream_reader(btAudioStreamReceived, true);
  a2dpSink.set_on_data_received(btAudioDataReceived);
  a2dpSink.set_rssi_active(true);
  a2dpSink.set_rssi_callback(btRssiReceived);
  a2dpSink.start(BT_AUDIO_NAME);

  btAudioStarted = true;
  btStartedAtMs = millis();
}

bool isBluetoothStreaming() {
  if (!btAudioStarted) return false;
  if (a2dpSink.is_output_active()) return true;
  if (btLastAudioMs == 0) return false;

  return millis() - btLastAudioMs < BT_STREAM_TIMEOUT_MS;
}

const char* getBluetoothStatusText() {
  if (!btAudioStarted) return "OFF";
  if (isBluetoothStreaming()) return "PLAY";
  if (a2dpSink.is_connected()) return "CONN";
  return "PAIR";
}

void updateBluetoothRssi() {
  if (!btAudioStarted || !a2dpSink.is_connected()) {
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
  if (!btAudioStarted || !a2dpSink.is_connected()) return 0;

  if (!btRssiValid) {
    return isBluetoothStreaming() ? 3 : 2;
  }

  int delta = abs((int)btRssiDelta);

  if (delta <= 5) return 4;
  if (delta <= 15) return 3;
  if (delta <= 30) return 2;
  return 1;
}

int getActiveMenuCount() {
  return areaChoices[activeType].itemCount;
}

int getMenuItemIndex(int visibleIndex) {
  int count = getActiveMenuCount();

  if (visibleIndex < 0 || visibleIndex >= count) {
    return 0;
  }

  return areaChoices[activeType].items[visibleIndex];
}

MenuAction getSelectedAction() {
  int itemIndex = getMenuItemIndex(selectedItem);
  return menuItems[itemIndex].action;
}

void wrapSelectedMenu() {
  int count = getActiveMenuCount();

  if (selectedItem < 0) selectedItem = count - 1;
  if (selectedItem >= count) selectedItem = 0;

  if (selectedItem < topItem) {
    topItem = selectedItem;
  }

  if (selectedItem >= topItem + VISIBLE_ROWS) {
    topItem = selectedItem - VISIBLE_ROWS + 1;
  }
}

void wrapSelectedType() {
  if (selectedType < 0) selectedType = TYPE_COUNT - 1;
  if (selectedType >= TYPE_COUNT) selectedType = 0;
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

  selectedItem = 0;
  topItem = 0;
  wrapSelectedMenu();

  lastWaveform = -1;
  lastFrequency = -1;
  lastVolume = -1;
  lastAttack = -1;
  lastDecay = -1;
  lastWaveFold = -1;
  lastWavePhase = -1;
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
    case ACTION_WAVEFORM:
      snprintf(buffer, sizeof(buffer), "%s", waveformNames[waveform]);
      break;

    case ACTION_PITCH:
      snprintf(buffer, sizeof(buffer), "%dHz", frequency);
      break;

    case ACTION_LEVEL:
      snprintf(buffer, sizeof(buffer), "%d%%", volume);
      break;

    case ACTION_WAVE_FOLD:
      snprintf(buffer, sizeof(buffer), "%d%%", waveFold);
      break;

    case ACTION_WAVE_PHASE:
      snprintf(buffer, sizeof(buffer), "%ddeg", wavePhase);
      break;

    case ACTION_ATTACK:
      snprintf(buffer, sizeof(buffer), "%dms", attack);
      break;

    case ACTION_DECAY:
      snprintf(buffer, sizeof(buffer), "%dms", decay);
      break;

    case ACTION_SPECTRUM:
      snprintf(buffer, sizeof(buffer), "Live");
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
      snprintf(buffer, sizeof(buffer), "Pair");
      break;

    case ACTION_BT_I2S:
      snprintf(buffer, sizeof(buffer), "%d/%d/%d", PCM5102_BCK, PCM5102_LRCK, PCM5102_DIN);
      break;

    case ACTION_BT_OUTPUT:
      snprintf(buffer, sizeof(buffer), "AUX");
      break;

    case ACTION_INPUTS:
      snprintf(buffer, sizeof(buffer), "IO");
      break;

    case ACTION_DISPLAY_TEST:
      snprintf(buffer, sizeof(buffer), "LCD");
      break;

    case ACTION_ABOUT:
      snprintf(buffer, sizeof(buffer), "Info");
      break;

    default:
      snprintf(buffer, sizeof(buffer), "-");
      break;
  }

  return buffer;
}

void changeCurrentValue(int delta) {
  switch (getSelectedAction()) {
    case ACTION_WAVEFORM:
      waveform += delta;
      if (waveform < 0) waveform = 3;
      if (waveform > 3) waveform = 0;
      break;

    case ACTION_PITCH:
      frequency += delta * 5;
      frequency = clampInt(frequency, 20, 2000);
      break;

    case ACTION_LEVEL:
      volume += delta;
      volume = clampInt(volume, 0, 100);
      break;

    case ACTION_WAVE_FOLD:
      waveFold += delta * 2;
      waveFold = clampInt(waveFold, 0, 100);
      break;

    case ACTION_WAVE_PHASE:
      wavePhase += delta * 5;
      wavePhase = wrapIndex(wavePhase, 360);
      break;

    case ACTION_ATTACK:
      attack += delta;
      attack = clampInt(attack, 0, 500);
      break;

    case ACTION_DECAY:
      decay += delta;
      decay = clampInt(decay, 0, 2000);
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

    default:
      break;
  }

  valueDirty = true;
}

// =======================
// UI LOGIC
// =======================

void setMode(UiMode newMode) {
  uiMode = newMode;
  fullRedraw = true;
  valueDirty = true;
  lastBtHeaderRefreshMs = 0;
  lastMenuMoveMs = 0;
}

void updateUiLogic() {
  int32_t steps = getEncoderSteps();

  if (uiMode == MODE_BOOT) {
    if (pushEvent || k0Event || millis() - bootStartMs >= bootDurationMs) {
      pushEvent = false;
      k0Event = false;
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
      if (getSelectedAction() == ACTION_ABOUT) {
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

    else if (uiMode == MODE_MENU || uiMode == MODE_EDIT || uiMode == MODE_ABOUT) {
      setMode(MODE_PREMENU);
    }
  }
}

// =======================
// FAST DRAW HELPERS
// =======================

int textPixelWidth(const char* text, uint8_t size) {
  return strlen(text) * 6 * size;
}

uint16_t getAccentColor(int itemIndex) {
  if (itemIndex < 0) return COL_NOTE;
  return ITEM_COLORS[itemIndex % ITEM_COLOR_COUNT];
}

void printRight(const char* text, int rightX, int y, uint8_t size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.setCursor(rightX - textPixelWidth(text, size), y);
  tft.print(text);
}

float normalizePhase(float phase) {
  phase -= floorf(phase);
  if (phase < 0.0f) phase += 1.0f;
  return phase;
}

float applyWaveFold(float sample) {
  float drive = 1.0f + (waveFold / 42.0f);
  sample *= drive;

  while (sample > 1.0f) {
    sample = 2.0f - sample;
  }

  while (sample < -1.0f) {
    sample = -2.0f - sample;
  }

  return sample;
}

float getOscillatorSample(float phase) {
  phase = normalizePhase(phase + (wavePhase / 360.0f));
  float sample = 0.0f;

  switch (waveform) {
    case 0:
      sample = sinf(phase * TWO_PI_F);
      break;

    case 1:
      sample = phase < 0.5f ? 1.0f : -1.0f;
      break;

    case 2:
      sample = phase < 0.5f ? phase * 4.0f - 1.0f : 3.0f - phase * 4.0f;
      break;

    case 3:
      sample = phase * 2.0f - 1.0f;
      break;
  }

  return applyWaveFold(sample);
}

float getOscillatorRms() {
  float level = volume / 100.0f;
  float sum = 0.0f;

  for (int i = 0; i < 48; i++) {
    float sample = getOscillatorSample((float)i / 48.0f) * level;
    sum += sample * sample;
  }

  return sqrtf(sum / 48.0f);
}

int getRmsDbFs() {
  float rms = getOscillatorRms();
  if (rms < 0.0001f) return -80;

  int db = (int)(20.0f * log10f(rms));
  return clampInt(db, -80, 0);
}

float getHarmonicAmplitude(int harmonic) {
  if (harmonic <= 0) return 0.0f;
  float amplitude = 0.0f;

  switch (waveform) {
    case 0:
      amplitude = harmonic == 1 ? 1.0f : 0.0f;
      break;

    case 1:
      amplitude = (harmonic % 2) == 1 ? 1.0f / harmonic : 0.0f;
      break;

    case 2:
      amplitude = (harmonic % 2) == 1 ? 1.0f / ((float)harmonic * harmonic) : 0.0f;
      break;

    case 3:
      amplitude = 1.0f / harmonic;
      break;
  }

  if (waveFold > 0 && harmonic > 1) {
    float foldEnergy = (waveFold / 100.0f) * (1.0f / (harmonic * 1.35f));
    amplitude += foldEnergy;
  }

  return amplitude;
}

int getSpectrumBarCount() {
  return FFT_BIN_VALUES[fftBinsIndex];
}

float getSpectrumMaxHz() {
  return (float)FFT_RANGE_VALUES[fftRangeIndex];
}

const char* getSpectrumMaxLabel() {
  return FFT_RANGE_LABELS[fftRangeIndex];
}

float getWindowLeakage() {
  switch (fftWindow) {
    case 1:
      return 0.05f;

    case 2:
      return 0.20f;

    case 3:
      return 0.30f;
  }

  return 0.12f;
}

float getSpectrumPosition(float hz) {
  float safeHz = hz < SPECTRUM_MIN_HZ ? SPECTRUM_MIN_HZ : hz;
  float maxHz = getSpectrumMaxHz();

  if (safeHz > maxHz) safeHz = maxHz;

  float range = logf(maxHz / SPECTRUM_MIN_HZ);
  float position = logf(safeHz / SPECTRUM_MIN_HZ) / range;
  int bins = getSpectrumBarCount();

  return position * (bins - 1);
}

void buildOscillatorSpectrum(float magnitudes[], bool normalizeOutput) {
  int bins = getSpectrumBarCount();

  for (int i = 0; i < bins; i++) {
    magnitudes[i] = 0.0f;
  }

  float level = volume / 100.0f;
  float leakage = getWindowLeakage();
  float maxHz = getSpectrumMaxHz();

  for (int harmonic = 1; harmonic <= 96; harmonic++) {
    float hz = frequency * harmonic;
    if (hz > maxHz) break;

    float amplitude = getHarmonicAmplitude(harmonic) * level;
    if (amplitude <= 0.0f) continue;

    float position = getSpectrumPosition(hz);
    int left = (int)floorf(position);
    float blend = position - left;

    if (left >= 0 && left < bins) {
      magnitudes[left] += amplitude * (1.0f - blend);
    }

    if (left + 1 >= 0 && left + 1 < bins) {
      magnitudes[left + 1] += amplitude * blend;
    }

    if (left - 1 >= 0 && left - 1 < bins) {
      magnitudes[left - 1] += amplitude * leakage * 0.55f;
    }

    if (left + 2 >= 0 && left + 2 < bins) {
      magnitudes[left + 2] += amplitude * leakage * 0.35f;
    }
  }

  int smoothPasses = fftSmoothing / 35;

  for (int pass = 0; pass < smoothPasses; pass++) {
    float smoothed[SPECTRUM_MAX_BARS];

    for (int i = 0; i < bins; i++) {
      float left = i > 0 ? magnitudes[i - 1] : magnitudes[i];
      float right = i + 1 < bins ? magnitudes[i + 1] : magnitudes[i];
      smoothed[i] = (left + magnitudes[i] * 2.0f + right) * 0.25f;
    }

    for (int i = 0; i < bins; i++) {
      magnitudes[i] = smoothed[i];
    }
  }

  float maxMagnitude = normalizeOutput ? 0.0f : 1.0f;

  if (normalizeOutput) {
    for (int i = 0; i < bins; i++) {
      if (magnitudes[i] > maxMagnitude) maxMagnitude = magnitudes[i];
    }

    if (maxMagnitude < 0.001f) maxMagnitude = 1.0f;
  }

  for (int i = 0; i < bins; i++) {
    magnitudes[i] = clampInt((int)((magnitudes[i] / maxMagnitude) * 100.0f), 0, 100) / 100.0f;
  }
}

void getPitchLabel(char* buffer, size_t size) {
  const char* noteNames[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
  };

  float midi = 69.0f + 12.0f * (logf(frequency / 440.0f) / logf(2.0f));
  int nearest = (int)roundf(midi);
  int note = nearest % 12;
  if (note < 0) note += 12;

  int octave = nearest / 12 - 1;
  int cents = (int)roundf((midi - nearest) * 100.0f);

  snprintf(buffer, size, "%s%d %+dc", noteNames[note], octave, cents);
}

int getWavePreviewY(int step, int width, int y, int height) {
  if (width <= 1) return y + height / 2;

  float phase = (float)step / (float)(width - 1);
  int travel = height - 4;
  float value = getOscillatorSample(phase);

  return y + height / 2 - (int)(value * (travel / 2));
}

void drawWavePreview(int x, int y, int width, int height, uint16_t color) {
  int prevY = getWavePreviewY(0, width, y, height);

  for (int i = 1; i < width; i++) {
    int nextY = getWavePreviewY(i, width, y, height);
    tft.drawLine(x + i - 1, prevY, x + i, nextY, color);
    prevY = nextY;
  }
}

void drawBluetoothHeaderField(bool force) {
  uint32_t now = millis();

  if (!force && now - lastBtHeaderRefreshMs < BT_HEADER_REFRESH_MS) {
    return;
  }

  updateBluetoothRssi();

  bool connected = btAudioStarted && a2dpSink.is_connected();
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
    case ACTION_WAVEFORM:
      drawWavePreview(cx - 8, cy - 5, 16, 10, color);
      break;

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

    case ACTION_WAVE_FOLD:
      tft.drawLine(cx - 8, cy + 4, cx - 3, cy - 5, color);
      tft.drawLine(cx - 3, cy - 5, cx + 2, cy + 5, color);
      tft.drawLine(cx + 2, cy + 5, cx + 7, cy - 4, color);
      tft.drawFastHLine(cx - 8, cy, 16, rgb565(62, 68, 82));
      break;

    case ACTION_WAVE_PHASE:
      tft.drawCircle(cx, cy, 6, color);
      tft.drawLine(cx, cy, cx + 6, cy - 1, color);
      tft.fillCircle(cx, cy, 2, color);
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

    case ACTION_SPECTRUM:
      for (int i = 0; i < 5; i++) {
        int h = 3 + i * 2;
        tft.fillRect(cx - 8 + i * 4, cy + 6 - h, 2, h, color);
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

void drawWaveformTabs(int y, uint16_t accent) {
  const char* tabNames[] = { "SIN", "SQR", "TRI", "SAW" };

  for (int i = 0; i < 4; i++) {
    int x = 10 + i * 37;
    bool active = i == waveform;

    if (active) {
      tft.fillRect(x - 2, y - 2, 32, 12, accent);
      tft.setTextColor(COL_BG);
    } else {
      tft.drawRect(x - 2, y - 2, 32, 12, COL_LINE);
      tft.setTextColor(COL_MUTED);
    }

    tft.setTextSize(1);
    tft.setCursor(x + 1, y + 1);
    tft.print(tabNames[i]);
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

void drawTimbreEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "OSC", accent);
  drawPanelFrame(10, 74, 140, 32, accent);
  drawScopeGrid(10, 74, 140, 32);
  drawOscilloscopeTrace(14, 78, 132, 24, accent, false);
  drawWaveformTabs(114, accent);
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

void drawWaveFoldEdit(uint16_t accent) {
  char driveText[10];
  int driveTenths = 10 + (waveFold * 24) / 100;
  snprintf(driveText, sizeof(driveText), "x%d.%d", driveTenths / 10, driveTenths % 10);

  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "FOLD", accent);
  printRight(driveText, 150, 67, 1, COL_INK);

  drawPanelFrame(10, 74, 140, 34, accent);
  drawScopeGrid(10, 74, 140, 34);
  drawOscilloscopeTrace(14, 78, 132, 26, accent, true);

  tft.setTextSize(1);
  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 112);
  tft.print("CLEAN");
  printRight("FOLD", 150, 112, 1, COL_MUTED);
  drawGrooveBar(50, 115, 60, 8, map(waveFold, 0, 100, 0, 58), accent);
}

void drawWavePhaseEdit(uint16_t accent) {
  char phaseText[12];
  snprintf(phaseText, sizeof(phaseText), "%ddeg", wavePhase);

  drawValueReadout(phaseText, "PHASE", accent);

  drawPanelFrame(10, 74, 56, 48, accent);
  int cx = 38;
  int cy = 98;
  float angle = (wavePhase - 90) * TWO_PI_F / 360.0f;
  int mx = cx + (int)(cosf(angle) * 17.0f);
  int my = cy + (int)(sinf(angle) * 17.0f);

  tft.drawCircle(cx, cy, 18, COL_LINE);
  tft.drawCircle(cx, cy, 12, rgb565(34, 40, 53));
  tft.drawLine(cx, cy, mx, my, accent);
  tft.fillCircle(mx, my, 2, COL_GOLD);

  drawPanelFrame(74, 74, 76, 48, accent);
  drawScopeGrid(74, 74, 76, 48);
  drawOscilloscopeTrace(78, 80, 68, 35, accent, true);
}

void drawAttackEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "ENV A", accent);
  drawEnvelopeCurve(true, accent);
  drawGrooveBar(78, 116, 72, 8, map(attack, 0, 500, 0, 70), accent);
}

void drawDecayEdit(uint16_t accent) {
  drawValueReadout(getValueText(getMenuItemIndex(selectedItem)), "ENV D", accent);
  drawEnvelopeCurve(false, accent);
  drawGrooveBar(78, 116, 72, 8, map(decay, 0, 2000, 0, 70), accent);
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
  bool connected = a2dpSink.is_connected();
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
  drawValueReadout("PAIR", "NAME", accent);
  printRight(btAudioStarted ? "visible" : "starting", 150, 67, 1, btAudioStarted ? COL_INK : COL_CORAL);

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

// =======================
// BOOT / PREMENU DRAW
// =======================

void drawBootScreen() {
  uint32_t elapsed = millis() - bootStartMs;
  int progress = clampInt((int)((elapsed * 100) / bootDurationMs), 0, 100);

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
}

void drawPreMenuBase() {
  drawStageBackdrop();

  drawHeader("SELECT AREA");

  lastSelectedType = -1;
}

void drawTypeRow(int row, int typeIndex) {
  int y = 24 + row * 20;
  bool selected = typeIndex == selectedType;
  bool active = typeIndex == activeType;
  uint16_t accent = getAccentColor(typeIndex);
  uint16_t bg = selected ? COL_PANEL : COL_BG;

  tft.fillRect(0, y - 2, SCREEN_W, 20, bg);

  if (selected) {
    tft.fillTriangle(0, y - 2, 0, y + 18, 9, y + 8, accent);
    tft.drawFastVLine(154, y, 16, accent);
    tft.drawFastHLine(12, y + 17, 136, accent);
  } else {
    tft.drawFastHLine(12, y + 17, 136, rgb565(18, 23, 36));
  }

  tft.setTextSize(1);
  tft.setTextColor(selected ? accent : COL_MUTED);
  tft.setCursor(15, y + 5);
  tft.print(areaChoices[typeIndex].tag);

  tft.setTextColor(selected ? COL_INK : COL_MUTED);
  tft.setCursor(55, y + 5);
  tft.print(areaChoices[typeIndex].label);

  for (int i = 0; i < 4; i++) {
    int h = 3 + ((typeIndex + i) % 4) * 2;
    uint16_t barColor = selected ? accent : rgb565(58, 65, 82);
    tft.fillRect(124 + i * 5, y + 13 - h, 3, h, barColor);
  }

  if (active) {
    tft.fillCircle(147, y + 8, 2, COL_GOLD);
  }
}

void updatePreMenuFast() {
  if (fullRedraw || lastDrawnMode != MODE_PREMENU) {
    drawPreMenuBase();

    for (int row = 0; row < TYPE_COUNT; row++) {
      drawTypeRow(row, row);
    }

    lastSelectedType = selectedType;
    lastDrawnMode = MODE_PREMENU;
    fullRedraw = false;
    return;
  }

  if (selectedType != lastSelectedType) {
    drawTypeRow(lastSelectedType, lastSelectedType);
    drawTypeRow(selectedType, selectedType);
    lastSelectedType = selectedType;
  }
}

// =======================
// MENU DRAW
// =======================

void drawMenuBase() {
  drawStageBackdrop();

  drawHeader(areaChoices[activeType].tag);

  lastSelectedItem = -1;
  lastTopItem = -1;
}

void drawMenuRow(int row, int visibleIndex) {
  int itemIndex = getMenuItemIndex(visibleIndex);
  int y = 25 + row * 20;
  bool selected = visibleIndex == selectedItem;
  uint16_t accent = getAccentColor(itemIndex);
  uint16_t rowBg = selected ? COL_PANEL : COL_BG;

  tft.fillRect(0, y - 2, SCREEN_W, 20, rowBg);

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
  tft.setTextColor(selected ? accent : COL_MUTED);
  tft.setCursor(32, y + 2);
  tft.print(menuItems[itemIndex].tag);

  tft.setTextColor(selected ? COL_INK : COL_MUTED);
  tft.setCursor(32, y + 11);
  tft.print(menuItems[itemIndex].label);

  const char* value = getValueText(itemIndex);

  if (selected) {
    int valueW = textPixelWidth(value, 1) + 8;
    int chipX = SCREEN_W - valueW - 6;

    tft.fillRoundRect(chipX, y + 3, valueW, 13, 3, accent);
    tft.setTextColor(COL_BG);
    tft.setCursor(chipX + 4, y + 6);
    tft.print(value);
  } else {
    printRight(value, 152, y + 6, 1, COL_MUTED);
  }
}

void drawVisibleMenuRows() {
  for (int row = 0; row < VISIBLE_ROWS; row++) {
    int visibleIndex = topItem + row;

    if (visibleIndex >= getActiveMenuCount()) {
      tft.fillRect(0, 25 + row * 20, SCREEN_W, 20, COL_BG);
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
    int oldRow = lastSelectedItem - topItem;
    int newRow = selectedItem - topItem;

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
  lastWaveFold = -1;
  lastWavePhase = -1;
  lastFftRangeIndex = -1;
  lastFftWindow = -1;
  lastFftSmoothing = -1;
  lastFftBinsIndex = -1;
  lastFftPeakHold = -1;

  valueDirty = true;
}

bool editValueChanged() {
  if (waveform != lastWaveform) return true;
  if (frequency != lastFrequency) return true;
  if (volume != lastVolume) return true;
  if (attack != lastAttack) return true;
  if (decay != lastDecay) return true;
  if (waveFold != lastWaveFold) return true;
  if (wavePhase != lastWavePhase) return true;
  if (fftRangeIndex != lastFftRangeIndex) return true;
  if (fftWindow != lastFftWindow) return true;
  if (fftSmoothing != lastFftSmoothing) return true;
  if (fftBinsIndex != lastFftBinsIndex) return true;
  if ((int)fftPeakHold != lastFftPeakHold) return true;

  return false;
}

void saveLastValues() {
  lastWaveform = waveform;
  lastFrequency = frequency;
  lastVolume = volume;
  lastAttack = attack;
  lastDecay = decay;
  lastWaveFold = waveFold;
  lastWavePhase = wavePhase;
  lastFftRangeIndex = fftRangeIndex;
  lastFftWindow = fftWindow;
  lastFftSmoothing = fftSmoothing;
  lastFftBinsIndex = fftBinsIndex;
  lastFftPeakHold = (int)fftPeakHold;
}

void drawEditValue() {
  MenuAction action = getSelectedAction();
  bool dynamicBluetoothView = action == ACTION_BT_RECEIVER || action == ACTION_BT_OUTPUT;
  bool bluetoothStreaming = dynamicBluetoothView ? isBluetoothStreaming() : false;

  if (dynamicBluetoothView && !valueDirty && !editValueChanged()) {
    uint32_t now = millis();
    bool stateChanged = bluetoothStreaming != lastBtStreaming;

    if (!stateChanged && now - lastBtUiRefreshMs < BT_UI_REFRESH_MS) {
      return;
    }
  }

  if (!dynamicBluetoothView && action != ACTION_INPUTS && !valueDirty && !editValueChanged()) {
    return;
  }

  int itemIndex = getMenuItemIndex(selectedItem);
  uint16_t accent = getAccentColor(itemIndex);

  valueDirty = false;
  saveLastValues();

  if (dynamicBluetoothView) {
    lastBtStreaming = bluetoothStreaming;
    lastBtPacketDrawn = btPacketCount;
    lastBtUiRefreshMs = millis();
  }

  tft.fillRect(0, 53, SCREEN_W, SCREEN_H - 53, COL_BG);

  switch (action) {
    case ACTION_WAVEFORM:
      drawTimbreEdit(accent);
      break;

    case ACTION_PITCH:
      drawPitchEdit(accent);
      break;

    case ACTION_LEVEL:
      drawLevelEdit(accent);
      break;

    case ACTION_WAVE_FOLD:
      drawWaveFoldEdit(accent);
      break;

    case ACTION_WAVE_PHASE:
      drawWavePhaseEdit(accent);
      break;

    case ACTION_ATTACK:
      drawAttackEdit(accent);
      break;

    case ACTION_DECAY:
      drawDecayEdit(accent);
      break;

    case ACTION_SPECTRUM:
      drawSpectrumEdit(accent);
      break;

    case ACTION_FFT_RANGE:
      drawFftRangeEdit(accent);
      break;

    case ACTION_FFT_WINDOW:
      drawFftWindowEdit(accent);
      break;

    case ACTION_FFT_SMOOTH:
      drawFftSmoothEdit(accent);
      break;

    case ACTION_FFT_HOLD:
      drawFftHoldEdit(accent);
      break;

    case ACTION_FFT_BINS:
      drawFftBinsEdit(accent);
      break;

    case ACTION_SCOPE:
      drawScopeEdit(accent);
      break;

    case ACTION_BT_RECEIVER:
      drawBluetoothReceiverEdit(accent);
      break;

    case ACTION_BT_DEVICE:
      drawBluetoothDeviceEdit(accent);
      break;

    case ACTION_BT_I2S:
      drawBluetoothI2SEdit(accent);
      break;

    case ACTION_BT_OUTPUT:
      drawBluetoothOutputEdit(accent);
      break;

    case ACTION_INPUTS:
      drawInputsEdit(accent);
      break;

    case ACTION_DISPLAY_TEST:
      drawDisplayTestEdit(accent);
      break;

    default:
      break;
  }
}

void updateEditFast() {
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

  drawHeader("STAGE NOTES");

  tft.setTextColor(COL_GOLD);
  tft.setTextSize(2);
  tft.setCursor(8, 30);
  tft.print("POLYGLIDER");

  tft.setTextColor(COL_NOTE);
  tft.setTextSize(1);
  tft.setCursor(10, 53);
  tft.print("LCD JAM LAB");

  tft.setTextColor(COL_INK);
  tft.setCursor(10, 68);
  tft.print("ESP32 + ST7735S");

  tft.setTextColor(COL_MUTED);
  tft.setCursor(10, 81);
  tft.print("encoder conducts UI");

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

  drawBluetoothHeaderField(false);
}

// =======================
// SETUP
// =======================

void setup() {
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

  // Se è stabile, prova 40000000.
  // Se vedi glitch o pixel strani, torna a 27000000.
  tft.setSPISpeed(40000000);

  tft.fillScreen(COL_BG);

  bootStartMs = millis();
  uiMode = MODE_BOOT;
  fullRedraw = true;

  drawBootScreen();
  startBluetoothAudio();

  Serial.println("POLYGLIDER LCD JAM UI START");
}

// =======================
// LOOP
// =======================

void loop() {
  updateButtons();
  updateUiLogic();
  drawUi();
}
