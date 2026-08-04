#pragma once

#include <Arduino.h>

// Sources and destinations deliberately live outside the UI and DSP layers.
// Adding Velocity, Mod Wheel, Aftertouch or LFO sources later only requires a
// new source producer and more rows in the same routing table.
enum EnvelopeId : uint8_t {
  ENVELOPE_ENV1,
  ENVELOPE_ENV2,
  ENVELOPE_ENV3,
  ENVELOPE_COUNT
};

enum ModSourceId : uint8_t {
  MOD_SOURCE_ENV1,
  MOD_SOURCE_ENV2,
  MOD_SOURCE_ENV3,
  MOD_SOURCE_LFO1,
  MOD_SOURCE_LFO2,
  MOD_SOURCE_COUNT
};

inline ModSourceId getEnvelopeModSource(int envelopeIndex) {
  if (envelopeIndex == ENVELOPE_ENV3) return MOD_SOURCE_ENV3;
  if (envelopeIndex == ENVELOPE_ENV2) return MOD_SOURCE_ENV2;
  return MOD_SOURCE_ENV1;
}

enum ModDestinationId : uint8_t {
  MOD_DEST_AMP_LEVEL,
  MOD_DEST_FILTER_CUTOFF,
  MOD_DEST_FILTER_RESONANCE,
  MOD_DEST_OSC1_PITCH,
  MOD_DEST_OSC2_PITCH,
  MOD_DEST_OSC3_PITCH,
  MOD_DEST_ALL_OSC_PITCH,
  MOD_DEST_OSC1_LEVEL,
  MOD_DEST_OSC2_LEVEL,
  MOD_DEST_OSC3_LEVEL,
  MOD_DEST_NOISE_LEVEL,
  MOD_DEST_OSC1_PULSE_WIDTH,
  MOD_DEST_OSC2_PULSE_WIDTH,
  MOD_DEST_OSC3_PULSE_WIDTH,
  MOD_DEST_OSC1_WAVEFOLD,
  MOD_DEST_OSC2_WAVEFOLD,
  MOD_DEST_OSC3_WAVEFOLD,
  MOD_DEST_PAN,
  MOD_DEST_DELAY_MIX,
  MOD_DEST_REVERB_MIX,
  MOD_DESTINATION_COUNT
};

struct ModDestinationInfo {
  const char* tag;
  const char* label;
};

const ModDestinationInfo MOD_DESTINATIONS[MOD_DESTINATION_COUNT] = {
  { "AMP", "Amp Level" },
  { "CUT", "Filter Cutoff" },
  { "RES", "Filter Resonance" },
  { "O1P", "Oscillator 1 Pitch" },
  { "O2P", "Oscillator 2 Pitch" },
  { "O3P", "Oscillator 3 Pitch" },
  { "ALLP", "All Osc Pitch" },
  { "O1L", "Oscillator 1 Level" },
  { "O2L", "Oscillator 2 Level" },
  { "O3L", "Oscillator 3 Level" },
  { "NOISE", "Noise Level" },
  { "O1PW", "Oscillator 1 PWM" },
  { "O2PW", "Oscillator 2 PWM" },
  { "O3PW", "Oscillator 3 PWM" },
  { "O1F", "Oscillator 1 Fold" },
  { "O2F", "Oscillator 2 Fold" },
  { "O3F", "Oscillator 3 Fold" },
  { "PAN", "Pan" },
  { "DLY", "Delay Mix" },
  { "REV", "Reverb Mix" }
};

struct AdsrParameters {
  int attackMs;
  int decayMs;
  int sustainPercent;
  int releaseMs;
};

struct ModRouteSettings {
  bool enabled;
  int8_t amountPercent;
};

enum LfoWaveformId : uint8_t {
  LFO_WAVE_SINE,
  LFO_WAVE_TRIANGLE,
  LFO_WAVE_SAW,
  LFO_WAVE_RAMP,
  LFO_WAVE_SQUARE,
  LFO_WAVE_SAMPLE_HOLD,
  LFO_WAVE_COUNT
};

const char* const LFO_WAVEFORM_NAMES[LFO_WAVE_COUNT] = {
  "SINE", "TRI", "SAW", "RAMP", "SQUARE", "S+H"
};

struct LfoControlParameters {
  bool enabled;
  uint8_t waveform;
  int frequencyCentiHz;
  int phasePercent;
  bool bipolar;
  int amountPercent;
  bool retriggerOnNoteOn;
  bool freeRunning;
  bool tempoSync;
  uint8_t tempoDivision;
  uint8_t performanceDestination;
};

inline int clampModulationAmount(int amount) {
  if (amount < -100) return -100;
  if (amount > 100) return 100;
  return amount;
}
