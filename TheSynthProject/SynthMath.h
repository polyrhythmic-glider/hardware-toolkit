#pragma once

#include <Arduino.h>
#include <math.h>

const float TWO_PI_F = 6.2831853f;

// Clamp per float usato da audio, envelope e visualizzazioni.
inline float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

// Smoothing one-pole leggero per evitare salti udibili nei parametri audio.
inline float smoothToward(float current, float target, float amount) {
  return current + (target - current) * amount;
}

inline float smoothAngleDegrees(float current, float target, float amount) {
  float delta = target - current;

  while (delta > 180.0f) delta -= 360.0f;
  while (delta < -180.0f) delta += 360.0f;

  current += delta * amount;

  while (current >= 360.0f) current -= 360.0f;
  while (current < 0.0f) current += 360.0f;

  return current;
}

inline float normalizePhase(float phase) {
  phase -= floorf(phase);
  if (phase < 0.0f) phase += 1.0f;
  return phase;
}

inline float applyWaveFoldAmount(float sample, float foldAmount) {
  float drive = 1.0f + (foldAmount / 42.0f);
  sample *= drive;

  while (sample > 1.0f) {
    sample = 2.0f - sample;
  }

  while (sample < -1.0f) {
    sample = -2.0f - sample;
  }

  return sample;
}

inline float getSawSample(float phase) {
  return phase * 2.0f - 1.0f;
}

inline float getNoiseSampleForPhase(float phase) {
  uint32_t bucket = (uint32_t)(normalizePhase(phase) * 64.0f);
  uint32_t hash = bucket * 1103515245UL + 12345UL;
  hash ^= hash >> 13;
  hash *= 1274126177UL;
  return ((hash & 0xFFFF) / 32767.5f) - 1.0f;
}

inline float getOscillatorSampleForParams(float phase, int shape, float foldAmount, float phaseDegrees) {
  phase = normalizePhase(phase + (phaseDegrees / 360.0f));
  float sample = 0.0f;
  float angle = phase * TWO_PI_F;

  switch (shape) {
    case 0:
      sample = sinf(angle);
      break;

    case 1:
      sample = phase < 0.5f ? 1.0f : -1.0f;
      break;

    case 2:
      sample = phase < 0.5f ? phase * 4.0f - 1.0f : 3.0f - phase * 4.0f;
      break;

    case 3:
      sample = getSawSample(phase);
      break;

    case 4:
      sample = 1.0f - phase * 2.0f;
      break;

    case 5:
      sample = phase < 0.25f ? 1.0f : -1.0f;
      break;

    case 6:
      sample = sinf(angle) * 0.65f + sinf(angle * 0.5f) * 0.35f;
      break;

    case 7:
      sample = sinf(angle) * 0.58f + sinf(angle * 2.0f) * 0.28f + sinf(angle * 3.0f) * 0.14f;
      break;

    case 8:
      sample = sinf(angle) * 0.55f + sinf(angle * 2.0f) * 0.45f;
      break;

    case 9:
      sample = sinf(angle) * 0.72f + sinf(angle * 3.0f) * 0.20f + sinf(angle * 5.0f) * 0.08f;
      break;

    case 10:
      sample = (getSawSample(phase) + getSawSample(normalizePhase(phase + 0.013f)) + getSawSample(normalizePhase(phase + 0.031f))) / 3.0f;
      break;

    case 11:
      sample = phase < 0.35f ? 1.0f : -1.0f;
      break;

    case 12:
    {
      const int steps = 8;
      int step = clampFloat(phase, 0.0f, 0.999f) * steps;
      sample = ((float)step / (float)(steps - 1)) * 2.0f - 1.0f;
      break;
    }

    case 13:
      sample = clampFloat(sinf(angle) * 1.8f, -0.8f, 0.8f) / 0.8f;
      break;

    case 14:
      sample = getNoiseSampleForPhase(phase);
      break;

    case 15:
      sample = sinf(angle) * 0.52f + sinf(angle * 3.0f) * 0.30f + sinf(angle * 7.0f) * 0.18f;
      break;

    default:
      sample = sinf(angle);
      break;
  }

  return applyWaveFoldAmount(sample, foldAmount);
}
