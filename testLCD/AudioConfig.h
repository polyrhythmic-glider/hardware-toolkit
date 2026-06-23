#pragma once

#include <Arduino.h>

const int SPECTRUM_MAX_BARS = 32;
const float SPECTRUM_MIN_HZ = 20.0f;

const int BT_FFT_SAMPLES = 128;
const int BT_FFT_BARS = 16;
const int BT_FFT_DECIMATE = 3;
const char* BT_AUDIO_NAME = "Polyrhythmic Glider";
const uint32_t BT_STREAM_TIMEOUT_MS = 1400;
const uint32_t BT_UI_REFRESH_MS = 250;
const uint32_t BT_RSSI_REFRESH_MS = 1800;
const uint32_t BT_HEADER_REFRESH_MS = 350;

// Volume A2DP: 0 = mute, 127 = massimo.
// 110 evita che Linux parta bassissimo, ma non spara subito al massimo.
const uint8_t BT_START_VOLUME = 110;
const uint32_t BT_START_VOLUME_RETRY_MS = 1200;

const int SYNTH_AUDIO_SAMPLE_RATE = 44100;
const int SYNTH_AUDIO_BUFFER_FRAMES = 128;
const float SYNTH_OUTPUT_GAIN = 0.30f;
const float SYNTH_DECLICK_AMOUNT = 0.018f;
const int SYNTH_AREA_INDEX = 0;
const int MIXER_AREA_INDEX = 1;
const int ENVELOPE_AREA_INDEX = 2;
const int SEQUENCER_AREA_INDEX = 3;
const int WAVE_VOICE_COUNT = 4;

const int DELAY_MAX_MS = 600;
const int DELAY_BUFFER_SAMPLES = (SYNTH_AUDIO_SAMPLE_RATE * DELAY_MAX_MS) / 1000;
const int REVERB_BUFFER_A_SAMPLES = 1557;
const int REVERB_BUFFER_B_SAMPLES = 1617;
const int REVERB_BUFFER_C_SAMPLES = 1491;

const int FILTER_MIN_CUTOFF_HZ = 40;
const int FILTER_MAX_CUTOFF_HZ = 12000;
const char* const FILTER_TYPE_NAMES[] = { "LP", "HP", "BP" };
const int FILTER_TYPE_COUNT = sizeof(FILTER_TYPE_NAMES) / sizeof(FILTER_TYPE_NAMES[0]);

const int FFT_RANGE_VALUES[] = { 1000, 4000, 8000, 12000 };
const char* const FFT_RANGE_LABELS[] = { "1K", "4K", "8K", "12K" };
const int FFT_RANGE_COUNT = sizeof(FFT_RANGE_VALUES) / sizeof(FFT_RANGE_VALUES[0]);

const int FFT_BIN_VALUES[] = { 12, 18, 24, 32 };
const int FFT_BIN_COUNT = sizeof(FFT_BIN_VALUES) / sizeof(FFT_BIN_VALUES[0]);

const char* const FFT_WINDOW_NAMES[] = { "HANN", "RECT", "BLACK", "FLAT" };
const int FFT_WINDOW_COUNT = sizeof(FFT_WINDOW_NAMES) / sizeof(FFT_WINDOW_NAMES[0]);
