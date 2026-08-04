#pragma once

#include <Preferences.h>
#include <stddef.h>

const uint32_t PRESET_MAGIC = 0x50474C44;
// Versione 7: quattro pagine da 16 step, pagina visibile e auto-scroll.
const uint16_t PRESET_VERSION = 7;
const uint16_t LEGACY_PRESET_VERSION_6 = 6;
const uint16_t LEGACY_PRESET_VERSION_5 = 5;
const uint16_t LEGACY_PRESET_VERSION = 4;
const uint16_t LEGACY_PRESET_VERSION_3 = 3;
const int PRESET_KICK_PATTERN_MAGIC = 0x4B49434B;
const int LEGACY_SEQUENCER_STEP_COUNT_16 = 16;
const int LEGACY_SEQUENCER_STEP_COUNT_8 = 8;
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
  int waveVoiceDetune[WAVE_VOICE_COUNT];
  int kickLevel;
  int kickPitch;
  int kickDecay;
  int kickSweep;
  int kickAttack;
  int kickDrive;
  int sequencerBpm;
  int sequencerLength;
  int reservedSequencerA;
  int reservedSequencerB;
  int reservedSequencerC;
  int sequencerStepPitches[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT];
  bool sequencerStepEnabled[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT];
  int reservedStepData[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT];
  int delayMix;
  int delayTimeMs;
  int delayFeedback;
  int reverbMix;
  int reverbRoom;
  AdsrParameters envelopeParameters[ENVELOPE_COUNT];
  ModRouteSettings modulationRoutes[MOD_SOURCE_COUNT][MOD_DESTINATION_COUNT];
};

// Layout v4 precedente all'introduzione della modulation matrix.
struct LegacySynthPresetV4 {
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
  int waveVoiceDetune[WAVE_VOICE_COUNT];
  int kickLevel;
  int kickPitch;
  int kickDecay;
  int kickSweep;
  int kickAttack;
  int kickDrive;
  int sequencerBpm;
  int sequencerLength;
  int reservedSequencerA;
  int reservedSequencerB;
  int reservedSequencerC;
  int sequencerStepNotes[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_16];
  bool sequencerStepEnabled[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_16];
  int reservedStepData[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_16];
  int delayMix;
  int delayTimeMs;
  int delayFeedback;
  int reverbMix;
  int reverbRoom;
};

// Layout v5/v6 a 16 step. La v5 memorizzava offset, la v6 pitch MIDI assoluti.
struct LegacySynthPresetV6 {
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
  int waveVoiceDetune[WAVE_VOICE_COUNT];
  int kickLevel;
  int kickPitch;
  int kickDecay;
  int kickSweep;
  int kickAttack;
  int kickDrive;
  int sequencerBpm;
  int sequencerLength;
  int reservedSequencerA;
  int reservedSequencerB;
  int reservedSequencerC;
  int sequencerStepPitches[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_16];
  bool sequencerStepEnabled[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_16];
  int reservedStepData[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_16];
  int delayMix;
  int delayTimeMs;
  int delayFeedback;
  int reverbMix;
  int reverbRoom;
  AdsrParameters envelopeParameters[ENVELOPE_COUNT];
  ModRouteSettings modulationRoutes[MOD_SOURCE_COUNT][MOD_DESTINATION_COUNT];
};

static_assert(offsetof(LegacySynthPresetV6, envelopeParameters) == sizeof(LegacySynthPresetV4),
              "Preset v5/v6 must retain the complete v4 prefix");
static_assert(offsetof(SynthPreset, sequencerStepPitches) ==
              offsetof(LegacySynthPresetV6, sequencerStepPitches),
              "Preset v7 must retain the scalar v6 prefix");

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
  int waveVoiceDetune[WAVE_VOICE_COUNT];
  int kickLevel;
  int kickPitch;
  int kickDecay;
  int kickSweep;
  int kickAttack;
  int kickDrive;
  int sequencerBpm;
  int sequencerLength;
  int reservedSequencerA;
  int reservedSequencerB;
  int reservedSequencerC;
  int sequencerStepNotes[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_8];
  bool sequencerStepEnabled[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_8];
  int reservedStepData[WAVE_VOICE_COUNT][LEGACY_SEQUENCER_STEP_COUNT_8];
  int delayMix;
  int delayTimeMs;
  int delayFeedback;
  int reverbMix;
  int reverbRoom;
};

void getPresetKey(char* key, size_t size) {
  snprintf(key, size, "slot%d", presetSlot);
}

int clampPresetInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

void resetDefaultKickPattern() {
  for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
    kickStepEnabled[step] = false;
  }
}

void getLegacyPitchMigration(int frequencyHz, int oldFineCents, int& basePitch, int& migratedFineCents) {
  float exactMidiPitch = frequencyToMidiPitch((float)frequencyHz);
  basePitch = (int)roundf(exactMidiPitch);
  migratedFineCents = oldFineCents + (int)roundf((exactMidiPitch - basePitch) * 100.0f);

  while (migratedFineCents > 50) {
    migratedFineCents -= 100;
    basePitch++;
  }
  while (migratedFineCents < -50) {
    migratedFineCents += 100;
    basePitch--;
  }
}

void convertLegacyStepOffsetsToPitches(LegacySynthPresetV6& preset) {
  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    int basePitch = SEQUENCER_DEFAULT_PITCH;
    int migratedFine = preset.waveVoiceDetune[voice];
    getLegacyPitchMigration(preset.frequency, preset.waveVoiceDetune[voice], basePitch, migratedFine);
    preset.waveVoiceDetune[voice] = clampPresetInt(migratedFine, -50, 50);

    for (int step = 0; step < LEGACY_SEQUENCER_STEP_COUNT_16; step++) {
      int oldOffset = preset.sequencerStepPitches[voice][step];
      preset.sequencerStepPitches[voice][step] = clampMidiPitch(basePitch + oldOffset);
    }
  }
}

void migrateLegacyPresetV6(const LegacySynthPresetV6& legacy, SynthPreset& migrated) {
  migrated = {};
  memcpy(&migrated, &legacy, offsetof(SynthPreset, sequencerStepPitches));
  migrated.version = PRESET_VERSION;
  migrated.reservedSequencerA = 1;  // auto-scroll enabled by default
  migrated.reservedSequencerB = 0;  // first page

  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      migrated.sequencerStepPitches[voice][step] = SEQUENCER_DEFAULT_PITCH;
      migrated.sequencerStepEnabled[voice][step] = false;
      migrated.reservedStepData[voice][step] = 0;
    }
    for (int step = 0; step < LEGACY_SEQUENCER_STEP_COUNT_16; step++) {
      migrated.sequencerStepPitches[voice][step] = legacy.sequencerStepPitches[voice][step];
      migrated.sequencerStepEnabled[voice][step] = legacy.sequencerStepEnabled[voice][step];
      migrated.reservedStepData[voice][step] = legacy.reservedStepData[voice][step];
    }
  }

  migrated.delayMix = legacy.delayMix;
  migrated.delayTimeMs = legacy.delayTimeMs;
  migrated.delayFeedback = legacy.delayFeedback;
  migrated.reverbMix = legacy.reverbMix;
  migrated.reverbRoom = legacy.reverbRoom;
  memcpy(migrated.envelopeParameters, legacy.envelopeParameters, sizeof(migrated.envelopeParameters));
  memcpy(migrated.modulationRoutes, legacy.modulationRoutes, sizeof(migrated.modulationRoutes));
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
  memcpy(preset.waveVoiceDetune, waveVoiceDetune, sizeof(waveVoiceDetune));
  preset.kickLevel = kickLevel;
  preset.kickPitch = kickPitch;
  preset.kickDecay = kickDecay;
  preset.kickSweep = kickSweep;
  preset.kickAttack = kickAttack;
  preset.kickDrive = kickDrive;
  preset.sequencerBpm = sequencerBpm;
  preset.sequencerLength = sequencerLength;
  preset.reservedSequencerA = sequencerAutoScroll ? 1 : 0;
  preset.reservedSequencerB = getSequencerPage();
  preset.reservedSequencerC = PRESET_KICK_PATTERN_MAGIC;
  memcpy(preset.sequencerStepPitches, sequencerStepPitches, sizeof(sequencerStepPitches));
  memcpy(preset.sequencerStepEnabled, sequencerStepEnabled, sizeof(sequencerStepEnabled));
  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      preset.reservedStepData[voice][step] = voice == 0 ? (kickStepEnabled[step] ? 1 : 0) : waveVoiceShape[voice];
    }
  }
  preset.delayMix = delayMix;
  preset.delayTimeMs = delayTimeMs;
  preset.delayFeedback = delayFeedback;
  preset.reverbMix = reverbMix;
  preset.reverbRoom = reverbRoom;
  memcpy(preset.envelopeParameters, envelopeParameters, sizeof(envelopeParameters));
  memcpy(preset.modulationRoutes, modulationRoutes, sizeof(modulationRoutes));
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
  memcpy(waveVoiceDetune, preset.waveVoiceDetune, sizeof(waveVoiceDetune));
  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    waveVoiceDetune[voice] = clampPresetInt(waveVoiceDetune[voice], -50, 50);
  }
  kickLevel = preset.kickLevel;
  kickPitch = preset.kickPitch;
  kickDecay = preset.kickDecay;
  kickSweep = preset.kickSweep;
  kickAttack = preset.kickAttack;
  kickDrive = preset.kickDrive;
  kickLevel = clampPresetInt(kickLevel, 0, 100);
  kickPitch = clampPresetInt(kickPitch, 25, 120);
  kickDecay = clampPresetInt(kickDecay, 80, 1200);
  kickSweep = clampPresetInt(kickSweep, 0, 260);
  kickAttack = clampPresetInt(kickAttack, 0, 100);
  kickDrive = clampPresetInt(kickDrive, 0, 100);
  sequencerBpm = preset.sequencerBpm;
  sequencerLength = preset.sequencerLength;
  sequencerAutoScroll = preset.reservedSequencerA != 0;
  sequencerPage = preset.reservedSequencerB;
  memcpy(sequencerStepPitches, preset.sequencerStepPitches, sizeof(sequencerStepPitches));
  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      sequencerStepPitches[voice][step] = clampMidiPitch(sequencerStepPitches[voice][step]);
    }
  }
  memcpy(sequencerStepEnabled, preset.sequencerStepEnabled, sizeof(sequencerStepEnabled));
  if (preset.reservedSequencerC == PRESET_KICK_PATTERN_MAGIC) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      kickStepEnabled[step] = preset.reservedStepData[0][step] != 0;
    }
  } else {
    resetDefaultKickPattern();
  }
  delayMix = preset.delayMix;
  delayTimeMs = preset.delayTimeMs;
  delayFeedback = preset.delayFeedback;
  reverbMix = preset.reverbMix;
  reverbRoom = preset.reverbRoom;
  memcpy(envelopeParameters, preset.envelopeParameters, sizeof(envelopeParameters));
  memcpy(modulationRoutes, preset.modulationRoutes, sizeof(modulationRoutes));
  for (int envelope = 0; envelope < ENVELOPE_COUNT; envelope++) {
    envelopeParameters[envelope].attackMs = clampPresetInt(envelopeParameters[envelope].attackMs, 0, 500);
    envelopeParameters[envelope].decayMs = clampPresetInt(envelopeParameters[envelope].decayMs, 0, 2000);
    envelopeParameters[envelope].sustainPercent = clampPresetInt(envelopeParameters[envelope].sustainPercent, 0, 100);
    envelopeParameters[envelope].releaseMs = clampPresetInt(envelopeParameters[envelope].releaseMs, 0, 3000);
  }
  for (int source = 0; source < MOD_SOURCE_COUNT; source++) {
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; destination++) {
      modulationRoutes[source][destination].amountPercent = (int8_t)clampModulationAmount(
        (int)modulationRoutes[source][destination].amountPercent
      );
    }
  }
  clampSequencerIndexes();
  resetSequencerClock();
}

void applyLegacyPresetV4(const LegacySynthPresetV4& preset) {
  LegacySynthPresetV6 expanded = {};
  memcpy(&expanded, &preset, sizeof(preset));
  expanded.envelopeParameters[ENVELOPE_ENV1] = {
    preset.attack, preset.decay, preset.sustain, preset.releaseMs
  };
  expanded.envelopeParameters[ENVELOPE_ENV2] = { 10, 80, 68, 320 };
  expanded.modulationRoutes[MOD_SOURCE_ENV1][MOD_DEST_AMP_LEVEL].enabled = true;
  expanded.modulationRoutes[MOD_SOURCE_ENV1][MOD_DEST_AMP_LEVEL].amountPercent = 100;
  convertLegacyStepOffsetsToPitches(expanded);

  SynthPreset migrated;
  migrateLegacyPresetV6(expanded, migrated);
  applyPreset(migrated);
}

void applyLegacyPresetV3(const LegacySynthPresetV3& preset) {
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
  memcpy(waveVoiceDetune, preset.waveVoiceDetune, sizeof(waveVoiceDetune));
  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    waveVoiceDetune[voice] = clampPresetInt(waveVoiceDetune[voice], -50, 50);
  }
  kickLevel = preset.kickLevel;
  kickPitch = preset.kickPitch;
  kickDecay = preset.kickDecay;
  kickSweep = preset.kickSweep;
  kickAttack = preset.kickAttack;
  kickDrive = preset.kickDrive;
  kickLevel = clampPresetInt(kickLevel, 0, 100);
  kickPitch = clampPresetInt(kickPitch, 25, 120);
  kickDecay = clampPresetInt(kickDecay, 80, 1200);
  kickSweep = clampPresetInt(kickSweep, 0, 260);
  kickAttack = clampPresetInt(kickAttack, 0, 100);
  kickDrive = clampPresetInt(kickDrive, 0, 100);
  sequencerBpm = preset.sequencerBpm;
  sequencerLength = preset.sequencerLength;
  sequencerAutoScroll = true;
  sequencerPage = 0;
  if (sequencerLength < 1) sequencerLength = 1;
  if (sequencerLength > LEGACY_SEQUENCER_STEP_COUNT_8) sequencerLength = LEGACY_SEQUENCER_STEP_COUNT_8;
  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    int basePitch = SEQUENCER_DEFAULT_PITCH;
    int migratedFine = preset.waveVoiceDetune[voice];
    getLegacyPitchMigration(preset.frequency, preset.waveVoiceDetune[voice], basePitch, migratedFine);
    waveVoiceDetune[voice] = clampPresetInt(migratedFine, -50, 50);
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      if (step < LEGACY_SEQUENCER_STEP_COUNT_8) {
        sequencerStepPitches[voice][step] = clampMidiPitch(basePitch + preset.sequencerStepNotes[voice][step]);
        sequencerStepEnabled[voice][step] = preset.sequencerStepEnabled[voice][step];
      } else {
        sequencerStepPitches[voice][step] = clampMidiPitch(basePitch);
        sequencerStepEnabled[voice][step] = false;
      }
    }
  }

  resetDefaultKickPattern();
  delayMix = preset.delayMix;
  delayTimeMs = preset.delayTimeMs;
  delayFeedback = preset.delayFeedback;
  reverbMix = preset.reverbMix;
  reverbRoom = preset.reverbRoom;
  envelopeParameters[ENVELOPE_ENV2] = { 10, 80, 68, 320 };
  initializeDefaultModulationRouting();
  clampSequencerIndexes();
  resetSequencerClock();
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

  if (storedSize == sizeof(LegacySynthPresetV6)) {
    LegacySynthPresetV6 preset;
    size_t read = preferences.getBytes(key, &preset, sizeof(preset));
    preferences.end();
    if (read != sizeof(preset) || preset.magic != PRESET_MAGIC ||
        (preset.version != LEGACY_PRESET_VERSION_5 &&
         preset.version != LEGACY_PRESET_VERSION_6)) return false;
    if (preset.version == LEGACY_PRESET_VERSION_5) {
      convertLegacyStepOffsetsToPitches(preset);
    }
    SynthPreset migrated;
    migrateLegacyPresetV6(preset, migrated);
    applyPreset(migrated);
    return true;
  }

  if (storedSize == sizeof(LegacySynthPresetV4)) {
    LegacySynthPresetV4 preset;
    size_t read = preferences.getBytes(key, &preset, sizeof(preset));
    preferences.end();
    if (read != sizeof(preset) || preset.magic != PRESET_MAGIC || preset.version != LEGACY_PRESET_VERSION) return false;
    applyLegacyPresetV4(preset);
    return true;
  }

  if (storedSize == sizeof(LegacySynthPresetV3)) {
    LegacySynthPresetV3 preset;
    size_t read = preferences.getBytes(key, &preset, sizeof(preset));
    preferences.end();
    if (read != sizeof(preset) || preset.magic != PRESET_MAGIC || preset.version != LEGACY_PRESET_VERSION_3) return false;
    applyLegacyPresetV3(preset);
    return true;
  }

  preferences.end();
  return false;
}
