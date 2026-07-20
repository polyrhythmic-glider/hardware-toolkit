#pragma once

#include <Preferences.h>

const uint32_t PRESET_MAGIC = 0x50474C44;
// Versione 4: il pattern del sequencer passa da 8 a 16 step.
const uint16_t PRESET_VERSION = 4;
const uint16_t LEGACY_PRESET_VERSION = 3;
const int LEGACY_SEQUENCER_STEP_COUNT = 8;
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
  int sequencerStepNotes[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT];
  bool sequencerStepEnabled[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT];
  int sequencerStepShapes[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT];
  int delayMix;
  int delayTimeMs;
  int delayFeedback;
  int reverbMix;
  int reverbRoom;
};

// Layout usato dalla versione 3, mantenuto per migrare i preset a 8 step.
struct LegacySynthPresetV3 {
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
  int sequencerStepNotes[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT];
  bool sequencerStepEnabled[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT];
  int sequencerStepShapes[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT];
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
  memcpy(preset.sequencerStepShapes, sequencerStepShapes, sizeof(sequencerStepShapes));
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
  memcpy(sequencerStepShapes, preset.sequencerStepShapes, sizeof(sequencerStepShapes));
  delayMix = preset.delayMix;
  delayTimeMs = preset.delayTimeMs;
  delayFeedback = preset.delayFeedback;
  reverbMix = preset.reverbMix;
  reverbRoom = preset.reverbRoom;
  clampSequencerIndexes();
  resetSequencerClock();
  synthRetriggerRequested = true;
}

void applyLegacyPreset(const LegacySynthPresetV3& preset) {
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
  if (sequencerLength < 1) sequencerLength = 1;
  if (sequencerLength > LEGACY_SEQUENCER_STEP_COUNT) sequencerLength = LEGACY_SEQUENCER_STEP_COUNT;
  sequencerDirection = preset.sequencerDirection;
  sequencerGate = preset.sequencerGate;
  sequencerTranspose = preset.sequencerTranspose;

  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      if (step < LEGACY_SEQUENCER_STEP_COUNT) {
        sequencerStepNotes[voice][step] = preset.sequencerStepNotes[voice][step];
        sequencerStepEnabled[voice][step] = preset.sequencerStepEnabled[voice][step];
        sequencerStepShapes[voice][step] = preset.sequencerStepShapes[voice][step];
      } else {
        sequencerStepNotes[voice][step] = 0;
        sequencerStepEnabled[voice][step] = false;
        sequencerStepShapes[voice][step] = waveVoiceShape[voice];
      }
    }
  }

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
  char key[12];
  getPresetKey(key, sizeof(key));
  Preferences preferences;
  if (!preferences.begin("polyglider", true)) return false;
  size_t storedSize = preferences.getBytesLength(key);

  if (storedSize == sizeof(SynthPreset)) {
    SynthPreset preset;
    size_t read = preferences.getBytes(key, &preset, sizeof(preset));
    preferences.end();
    if (read != sizeof(preset) || preset.magic != PRESET_MAGIC || preset.version != PRESET_VERSION) return false;
    applyPreset(preset);
    return true;
  }

  if (storedSize == sizeof(LegacySynthPresetV3)) {
    LegacySynthPresetV3 preset;
    size_t read = preferences.getBytes(key, &preset, sizeof(preset));
    preferences.end();
    if (read != sizeof(preset) || preset.magic != PRESET_MAGIC || preset.version != LEGACY_PRESET_VERSION) return false;
    applyLegacyPreset(preset);
    return true;
  }

  preferences.end();
  return false;
}
