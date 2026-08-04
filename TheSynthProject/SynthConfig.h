#pragma once

// Shared UI/control limits. Audio rendering runs exclusively on Daisy Seed.
constexpr int AUDIO_TEST_FREQUENCY_HZ = 440;
constexpr int DEFAULT_OUTPUT_VOLUME_PERCENT = 70;
constexpr int WAVE_VOICE_COUNT = 2;
constexpr int DELAY_MAX_MS = 600;

constexpr int FILTER_MIN_CUTOFF_HZ = 40;
constexpr int FILTER_MAX_CUTOFF_HZ = 12000;
const char* const FILTER_TYPE_NAMES[] = { "LP", "HP", "BP" };
constexpr int FILTER_TYPE_COUNT = sizeof(FILTER_TYPE_NAMES) / sizeof(FILTER_TYPE_NAMES[0]);
