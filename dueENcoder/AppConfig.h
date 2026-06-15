#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// ======================================================
//  HARDWARE CONFIG
// ======================================================

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int OLED_RESET = -1;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;

// ======================================================
//  UI CONFIG
// ======================================================

constexpr int UI_HEADER_HEIGHT = 16;
constexpr int UI_CONTENT_TOP = 17;
constexpr int UI_CONTENT_BOTTOM = 63;

constexpr float DEG_TO_RAD_FACTOR = 0.01745329251f;

enum UiMode {
  MODE_ENCODERS = 0,
  MODE_WAVEFORM = 1,
  MODE_PONG = 2
};

enum WaveformType {
  WAVE_SINE = 0,
  WAVE_TRI = 1,
  WAVE_STEP = 2,
  WAVE_CONST = 3
};

constexpr int MODE_COUNT = 3;

// ======================================================
//  ENCODER CONFIG
// ======================================================

constexpr int STEPS_PER_DETENT = 4;
constexpr unsigned long ENCODER_MIN_INTERVAL_US = 100;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 35;

// ======================================================
//  PONG CONFIG
// ======================================================

constexpr int PONG_PADDLE_HEIGHT = 14;
constexpr int PONG_PADDLE_HALF = 7;
constexpr int PONG_TOP = UI_CONTENT_TOP;
constexpr int PONG_BOTTOM = UI_CONTENT_BOTTOM;
constexpr int PONG_LEFT_X = 4;
constexpr int PONG_RIGHT_X = 123;

// ======================================================
//  SHARED UI STATE
// ======================================================

extern volatile bool valuesChanged;

extern unsigned long clickFlashUntil1;
extern unsigned long clickFlashUntil2;
extern unsigned long clickFlashUntil3;

extern UiMode currentMode;

extern bool menuOpen;
extern int menuSelection;
extern long menuStartValue3;

extern long parameterStartValue1;
extern long parameterStartValue2;

extern int waveformIndex;
extern int parameterA;
extern int parameterB;

#endif
