#pragma once

#include <Arduino.h>

// 1: ESP32 gestisce UI/controlli e invia lo stato alla Daisy via UART.
// 0: fallback al motore audio ESP32 + PCM5102 già esistente.
#ifndef SYNTH_AUDIO_BACKEND_DAISY
#define SYNTH_AUDIO_BACKEND_DAISY 0
#endif

const int SYNTH_AUDIO_SAMPLE_RATE = 44100;
const int SYNTH_AUDIO_BUFFER_FRAMES = 128;
const int AUDIO_TEST_FREQUENCY_HZ = 440;
const int16_t AUDIO_TEST_AMPLITUDE = 4096;
const int SYNTH_AUDIO_TASK_PRIORITY = 3;
const float SYNTH_OUTPUT_GAIN = 0.30f;
const float SYNTH_DECLICK_AMOUNT = 0.018f;
const float SYNTH_PARAMETER_SMOOTH_BLOCK = 0.16f;
const int SYNTH_STEP_EDGE_FADE_SAMPLES = 96;
const int SYNTH_SHAPE_CROSSFADE_SAMPLES = 96;
const int WAVE_VOICE_COUNT = 2;

const int DELAY_MAX_MS = 600;
const int DELAY_BUFFER_SAMPLES = (SYNTH_AUDIO_SAMPLE_RATE * DELAY_MAX_MS) / 1000;
const int REVERB_BUFFER_A_SAMPLES = 1557;
const int REVERB_BUFFER_B_SAMPLES = 1617;
const int REVERB_BUFFER_C_SAMPLES = 1491;

const int FILTER_MIN_CUTOFF_HZ = 40;
const int FILTER_MAX_CUTOFF_HZ = 12000;
const char* const FILTER_TYPE_NAMES[] = { "LP", "HP", "BP" };
const int FILTER_TYPE_COUNT = sizeof(FILTER_TYPE_NAMES) / sizeof(FILTER_TYPE_NAMES[0]);
