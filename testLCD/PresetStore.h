#pragma once

#include <Preferences.h>

const uint32_t PRESET_MAGIC = 0x50474C44;
const uint16_t PRESET_VERSION = 1;
const int PRESET_SLOT_COUNT = 8;

struct SynthPreset {
  uint32_t magic;
  uint16_t version;
  int frequency;
  int volume;
  int attack;
  int decay;
  int sustain;
  int releaseMs;
  bool filterEnabled;
  int filterType;
  int filterCutoff;
  int waveVoiceLevels[WAVE_VOICE_COUNT];
  int waveVoiceShape[WAVE_VOICE_COUNT];
  int waveVoiceTune[WAVE_VOICE_COUNT];
  int waveVoiceFold[WAVE_VOICE_COUNT];
  int waveVoicePhase[WAVE_VOICE_COUNT];
  int kickLevel;
  int kickPitch;
  int kickDecay;
  int kickSweep;
  int kickClick;
  int kickDrive;
  int sequencerBpm;
  int sequencerLength;
  int sequencerDirection;
  int sequencerGate;
  int sequencerTranspose;
  int sequencerStepNotes[SEQUENCER_STEP_COUNT];
  bool sequencerStepEnabled[SEQUENCER_STEP_COUNT];
  int delayMix;
  int delayTimeMs;
  int delayFeedback;
  int reverbMix;
  int reverbRoom;
};

void getPresetKey(char* key, size_t size) {
  snprintf(key, size, "slot%d", presetSlot);
}

void capturePreset(SynthPreset& preset) {
  preset.magic = PRESET_MAGIC;
  preset.version = PRESET_VERSION;
  preset.frequency = frequency;
  preset.volume = volume;
  preset.attack = attack;
  preset.decay = decay;
  preset.sustain = sustain;
  preset.releaseMs = releaseMs;
  preset.filterEnabled = filterEnabled;
  preset.filterType = filterType;
  preset.filterCutoff = filterCutoff;
  memcpy(preset.waveVoiceLevels, waveVoiceLevels, sizeof(waveVoiceLevels));
  memcpy(preset.waveVoiceShape, waveVoiceShape, sizeof(waveVoiceShape));
  memcpy(preset.waveVoiceTune, waveVoiceTune, sizeof(waveVoiceTune));
  memcpy(preset.waveVoiceFold, waveVoiceFold, sizeof(waveVoiceFold));
  memcpy(preset.waveVoicePhase, waveVoicePhase, sizeof(waveVoicePhase));
  preset.kickLevel = kickLevel;
  preset.kickPitch = kickPitch;
  preset.kickDecay = kickDecay;
  preset.kickSweep = kickSweep;
  preset.kickClick = kickClick;
  preset.kickDrive = kickDrive;
  preset.sequencerBpm = sequencerBpm;
  preset.sequencerLength = sequencerLength;
  preset.sequencerDirection = sequencerDirection;
  preset.sequencerGate = sequencerGate;
  preset.sequencerTranspose = sequencerTranspose;
  memcpy(preset.sequencerStepNotes, sequencerStepNotes, sizeof(sequencerStepNotes));
  memcpy(preset.sequencerStepEnabled, sequencerStepEnabled, sizeof(sequencerStepEnabled));
  preset.delayMix = delayMix;
  preset.delayTimeMs = delayTimeMs;
  preset.delayFeedback = delayFeedback;
  preset.reverbMix = reverbMix;
  preset.reverbRoom = reverbRoom;
}

void applyPreset(const SynthPreset& preset) {
  frequency = preset.frequency;
  volume = preset.volume;
  attack = preset.attack;
  decay = preset.decay;
  sustain = preset.sustain;
  releaseMs = preset.releaseMs;
  filterEnabled = preset.filterEnabled;
  filterType = preset.filterType;
  filterCutoff = preset.filterCutoff;
  memcpy(waveVoiceLevels, preset.waveVoiceLevels, sizeof(waveVoiceLevels));
  memcpy(waveVoiceShape, preset.waveVoiceShape, sizeof(waveVoiceShape));
  memcpy(waveVoiceTune, preset.waveVoiceTune, sizeof(waveVoiceTune));
  memcpy(waveVoiceFold, preset.waveVoiceFold, sizeof(waveVoiceFold));
  memcpy(waveVoicePhase, preset.waveVoicePhase, sizeof(waveVoicePhase));
  kickLevel = preset.kickLevel;
  kickPitch = preset.kickPitch;
  kickDecay = preset.kickDecay;
  kickSweep = preset.kickSweep;
  kickClick = preset.kickClick;
  kickDrive = preset.kickDrive;
  sequencerBpm = preset.sequencerBpm;
  sequencerLength = preset.sequencerLength;
  sequencerDirection = preset.sequencerDirection;
  sequencerGate = preset.sequencerGate;
  sequencerTranspose = preset.sequencerTranspose;
  memcpy(sequencerStepNotes, preset.sequencerStepNotes, sizeof(sequencerStepNotes));
  memcpy(sequencerStepEnabled, preset.sequencerStepEnabled, sizeof(sequencerStepEnabled));
  delayMix = preset.delayMix;
  delayTimeMs = preset.delayTimeMs;
  delayFeedback = preset.delayFeedback;
  reverbMix = preset.reverbMix;
  reverbRoom = preset.reverbRoom;
  clampSequencerIndexes();
  resetSequencerClock();
  synthRetriggerRequested = true;
}

bool saveCurrentPreset() {
  SynthPreset preset;
  capturePreset(preset);
  char key[12];
  getPresetKey(key, sizeof(key));
  Preferences preferences;
  if (!preferences.begin("polyglider", false)) return false;
  size_t written = preferences.putBytes(key, &preset, sizeof(preset));
  preferences.end();
  return written == sizeof(preset);
}

bool loadCurrentPreset() {
  SynthPreset preset;
  char key[12];
  getPresetKey(key, sizeof(key));
  Preferences preferences;
  if (!preferences.begin("polyglider", true)) return false;
  size_t storedSize = preferences.getBytesLength(key);
  size_t read = storedSize == sizeof(preset) ? preferences.getBytes(key, &preset, sizeof(preset)) : 0;
  preferences.end();
  if (read != sizeof(preset) || preset.magic != PRESET_MAGIC || preset.version != PRESET_VERSION) return false;
  applyPreset(preset);
  return true;
}
