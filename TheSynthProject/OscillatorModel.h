#pragma once

#include "SynthConfig.h"

// One independent bank of three oscillators per melodic sequencer voice.
constexpr int SYNTH_MELODIC_VOICE_COUNT = 2;
struct OscillatorBank {
  int masterLevelPercent = 100;
  bool oscillatorEnabled[SYNTH_OSCILLATOR_COUNT] = { true, false, false };
  int waveVoiceLevels[SYNTH_OSCILLATOR_COUNT] = { 100, 100, 0 };
  int waveVoiceShape[SYNTH_OSCILLATOR_COUNT] = { 0, 1, 0 };
  int waveVoiceTune[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
  int waveVoiceFold[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
  int waveVoiceDetune[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
  int oscillatorOctave[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
  int oscillatorPhasePercent[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
  int oscillatorPulseWidthPercent[SYNTH_OSCILLATOR_COUNT] = { 50, 50, 50 };
  int oscillatorPanPercent[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
  int oscillatorKeyTrackingPercent[SYNTH_OSCILLATOR_COUNT] = { 100, 100, 100 };
  int oscillatorModulationAmountPercent[SYNTH_OSCILLATOR_COUNT] = { 100, 100, 100 };
};
