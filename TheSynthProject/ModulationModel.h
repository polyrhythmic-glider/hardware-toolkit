#pragma once

#include <Arduino.h>

// Sources and destinations deliberately live outside the UI and DSP layers.
// Adding Velocity, Mod Wheel, Aftertouch or LFO sources later only requires a
// new source producer and more rows in the same routing table.
enum EnvelopeId : uint8_t {
  ENVELOPE_ENV1,
  ENVELOPE_ENV2,
  ENVELOPE_COUNT
};

enum ModSourceId : uint8_t {
  MOD_SOURCE_ENV1,
  MOD_SOURCE_ENV2,
  MOD_SOURCE_COUNT
};

inline ModSourceId getEnvelopeModSource(int envelopeIndex) {
  return envelopeIndex == ENVELOPE_ENV2 ? MOD_SOURCE_ENV2 : MOD_SOURCE_ENV1;
}

enum ModDestinationId : uint8_t {
  MOD_DEST_AMP_LEVEL,
  MOD_DEST_FILTER_CUTOFF,
  MOD_DEST_FILTER_RESONANCE,
  MOD_DEST_OSC1_PITCH,
  MOD_DEST_OSC2_PITCH,
  MOD_DEST_OSC_MIX,
  MOD_DEST_PWM,
  MOD_DEST_FM_AMOUNT,
  MOD_DEST_WAVEFOLD_AMOUNT,
  MOD_DEST_DRIVE,
  MOD_DEST_PAN,
  MOD_DEST_LFO1_RATE,
  MOD_DEST_LFO1_DEPTH,
  MOD_DEST_LFO2_RATE,
  MOD_DEST_LFO2_DEPTH,
  MOD_DEST_FX_MIX,
  MOD_DEST_DELAY_FEEDBACK,
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
  { "MIX", "Oscillator Mix" },
  { "PWM", "PWM" },
  { "FM", "FM Amount" },
  { "FOLD", "Wavefold Amount" },
  { "DRV", "Drive" },
  { "PAN", "Pan" },
  { "L1R", "LFO1 Rate" },
  { "L1D", "LFO1 Depth" },
  { "L2R", "LFO2 Rate" },
  { "L2D", "LFO2 Depth" },
  { "FX", "FX Mix" },
  { "DFB", "Delay Feedback" },
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

inline int clampModulationAmount(int amount) {
  if (amount < -100) return -100;
  if (amount > 100) return 100;
  return amount;
}
