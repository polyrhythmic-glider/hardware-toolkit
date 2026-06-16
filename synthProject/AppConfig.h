#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// ======================================================
//  HARDWARE CONFIG
// ======================================================

constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int OLED_RESET = -1;

constexpr uint8_t PCA9548A_ADDRESS = 0x70;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t DISPLAY_COUNT = 4;
constexpr uint32_t I2C_CLOCK_HZ = 400000;
constexpr bool DIRECT_OLED_TEST = false;
constexpr bool PCA_MUX_TEST = false;
constexpr bool MULTI_SCREEN_UI = true;
constexpr bool WIDE_PONG = true;
constexpr unsigned long STARTUP_LOGO_MS = 2400;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;

constexpr int AUDIO_I2S_BCK = 5;
constexpr int AUDIO_I2S_LRCK = 15;
constexpr int AUDIO_I2S_DIN = 4;
constexpr int AUDIO_SAMPLE_RATE = 44100;
constexpr int AUDIO_BUFFER_SAMPLES = 256;
constexpr float AUDIO_BASE_FREQUENCY = 220.0f;

// ======================================================
//  DEBUG CONFIG
// ======================================================

constexpr bool DEBUG_SERIAL = false;
constexpr unsigned long DEBUG_SERIAL_BAUD = 115200;
constexpr unsigned long DEBUG_HEARTBEAT_INTERVAL_MS = 1000;
constexpr unsigned long DEBUG_TOGGLE_HOLD_MS = 1000;

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
  MODE_PONG = 2,
  MODE_TEST = 3
};

enum WaveformType {
  WAVE_SINE = 0,
  WAVE_TRI = 1,
  WAVE_STEP = 2,
  WAVE_CONST = 3
};

constexpr int MODE_COUNT = 4;
constexpr int MENU_VISIBLE_ITEMS = 3;

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
constexpr int PONG_WORLD_WIDTH = WIDE_PONG ? (SCREEN_WIDTH * DISPLAY_COUNT) : SCREEN_WIDTH;
constexpr int PONG_LEFT_X = 4;
constexpr int PONG_RIGHT_X = PONG_WORLD_WIDTH - 5;
constexpr float PONG_SPEED_MULTIPLIER = 4.0f;
constexpr float PONG_BASE_X_SPEED = 1.45f * PONG_SPEED_MULTIPLIER;
constexpr float PONG_BASE_Y_SPEED = 0.75f * PONG_SPEED_MULTIPLIER;
constexpr float PONG_MAX_X_SPEED = 3.0f * PONG_SPEED_MULTIPLIER;
constexpr float PONG_MAX_Y_SPEED = 2.4f * PONG_SPEED_MULTIPLIER;

// ======================================================
//  SHARED UI STATE
// ======================================================

extern volatile bool valuesChanged;
extern bool debugMode;

extern unsigned long clickFlashUntil1;
extern unsigned long clickFlashUntil2;
extern unsigned long clickFlashUntil3;
extern unsigned long clickFlashUntil4;

extern UiMode currentMode;

extern bool menuOpen;
extern int menuSelection;
extern long menuStartValue4;

extern long parameterStartValue1;
extern long parameterStartValue2;

extern int waveformIndex;
extern int parameterA;
extern int parameterB;

#endif
