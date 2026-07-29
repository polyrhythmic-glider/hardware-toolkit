#pragma once

#include <Arduino.h>

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
const int AUDIO_TEST_FREQUENCY_HZ = 440;
const int16_t AUDIO_TEST_AMPLITUDE = 4096;
// Keep several rendered blocks queued. AudioTools maps buffer_size differently
// on the legacy and IDF 5 drivers, so this deliberately uses a unit-neutral
// name. 8 x 128 also matches the stable PCM5102 example more closely.
const int SYNTH_I2S_DMA_BUFFER_COUNT = 8;
const int SYNTH_I2S_DMA_BUFFER_SIZE = 128;
const int SYNTH_AUDIO_TASK_PRIORITY = 3;
const float SYNTH_OUTPUT_GAIN = 0.30f;
const float SYNTH_DECLICK_AMOUNT = 0.018f;
const float SYNTH_PARAMETER_SMOOTH_BLOCK = 0.16f;
const int SYNTH_STEP_EDGE_FADE_SAMPLES = 96;
const int SYNTH_SHAPE_CROSSFADE_SAMPLES = 96;
const int SYNTH_AREA_INDEX = 0;
const int MIXER_AREA_INDEX = 1;
const int ENVELOPE_AREA_INDEX = 2;
const int SEQUENCER_AREA_INDEX = 3;
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
