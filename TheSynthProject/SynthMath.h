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

inline float normalizePhase(float phase) {
  phase -= (int32_t)phase;
  if (phase < 0.0f) phase += 1.0f;
  return phase;
}

inline float getFastSineSample(float phase) {
  // ESP32 uses this only for UI previews; realtime oscillator rendering lives
  // on Daisy and therefore needs no lookup table or audio-specific state here.
  return sinf(normalizePhase(phase) * TWO_PI_F);
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

inline float getOscillatorSampleForModParams(float phase, int shape, float foldAmount, float phaseDegrees, float pulseWidth) {
  phase = normalizePhase(phase + (phaseDegrees / 360.0f));
  pulseWidth = clampFloat(pulseWidth, 0.05f, 0.95f);
  float sample = 0.0f;

  switch (shape) {
    case 0:
      sample = getFastSineSample(phase);
      break;

    case 1:
      sample = phase < pulseWidth ? 1.0f : -1.0f;
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
      sample = phase < clampFloat(pulseWidth * 0.5f, 0.05f, 0.45f) ? 1.0f : -1.0f;
      break;

    case 6:
      sample = getFastSineSample(phase) * 0.65f + getFastSineSample(phase * 0.5f) * 0.35f;
      break;

    case 7:
      sample = getFastSineSample(phase) * 0.58f + getFastSineSample(phase * 2.0f) * 0.28f + getFastSineSample(phase * 3.0f) * 0.14f;
      break;

    case 8:
      sample = getFastSineSample(phase) * 0.55f + getFastSineSample(phase * 2.0f) * 0.45f;
      break;

    case 9:
      sample = getFastSineSample(phase) * 0.72f + getFastSineSample(phase * 3.0f) * 0.20f + getFastSineSample(phase * 5.0f) * 0.08f;
      break;

    case 10:
      sample = (getSawSample(phase) + getSawSample(normalizePhase(phase + 0.013f)) + getSawSample(normalizePhase(phase + 0.031f))) / 3.0f;
      break;

    case 11:
      sample = phase < pulseWidth ? 1.0f : -1.0f;
      break;

    case 12:
    {
      const int steps = 8;
      int step = clampFloat(phase, 0.0f, 0.999f) * steps;
      sample = ((float)step / (float)(steps - 1)) * 2.0f - 1.0f;
      break;
    }

    case 13:
      sample = clampFloat(getFastSineSample(phase) * 1.8f, -0.8f, 0.8f) / 0.8f;
      break;

    case 14:
      sample = getNoiseSampleForPhase(phase);
      break;

    case 15:
      sample = getFastSineSample(phase) * 0.52f + getFastSineSample(phase * 3.0f) * 0.30f + getFastSineSample(phase * 7.0f) * 0.18f;
      break;

    default:
      sample = getFastSineSample(phase);
      break;
  }

  return applyWaveFoldAmount(sample, foldAmount);
}

inline float getOscillatorSampleForParams(float phase, int shape, float foldAmount, float phaseDegrees) {
  return getOscillatorSampleForModParams(phase, shape, foldAmount, phaseDegrees, 0.5f);
}
