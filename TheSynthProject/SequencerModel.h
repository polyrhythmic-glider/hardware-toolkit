#pragma once

#include <Arduino.h>

const int SEQUENCER_STEP_COUNT = 16;
const int SEQUENCER_MIN_PITCH = 0;
const int SEQUENCER_MAX_PITCH = 127;
const int SEQUENCER_DEFAULT_PITCH = 57;  // A3 = 220 Hz
const int SEQUENCER_TRACK_VOICE_1 = 0;
const int SEQUENCER_TRACK_VOICE_2 = 1;
const int SEQUENCER_TRACK_KICK = 2;
const int SEQUENCER_TRACK_COUNT = 3;

const char* const MIDI_NOTE_NAMES[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

inline int clampMidiPitch(int midiPitch) {
  if (midiPitch < SEQUENCER_MIN_PITCH) return SEQUENCER_MIN_PITCH;
  if (midiPitch > SEQUENCER_MAX_PITCH) return SEQUENCER_MAX_PITCH;
  return midiPitch;
}

inline float midiPitchToFrequency(float midiPitch) {
  return 440.0f * powf(2.0f, (midiPitch - 69.0f) / 12.0f);
}

inline float frequencyToMidiPitch(float frequencyHz) {
  if (frequencyHz < 0.01f) frequencyHz = 0.01f;
  return 69.0f + 12.0f * (logf(frequencyHz / 440.0f) / logf(2.0f));
}

inline void formatMidiPitch(int midiPitch, char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  midiPitch = clampMidiPitch(midiPitch);
  snprintf(output, outputSize, "%s%d", MIDI_NOTE_NAMES[midiPitch % 12], midiPitch / 12 - 1);
}
