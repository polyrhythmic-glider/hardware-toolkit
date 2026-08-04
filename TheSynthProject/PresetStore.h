#pragma once

#include <Preferences.h>
#include <stddef.h>

const uint32_t PRESET_MAGIC = 0x50474C44;
// Versione 8: patch Daisy moderno (OSC3, ENV3, LFO e route estese).
const uint16_t PRESET_VERSION = 8;
const uint16_t LEGACY_PRESET_VERSION_7 = 7;
const uint16_t LEGACY_PRESET_VERSION_6 = 6;
const uint16_t LEGACY_PRESET_VERSION_5 = 5;
const uint16_t LEGACY_PRESET_VERSION = 4;
const uint16_t LEGACY_PRESET_VERSION_3 = 3;
const int PRESET_KICK_PATTERN_MAGIC = 0x4B49434B;
const int LEGACY_SEQUENCER_STEP_COUNT_16 = 16;
const int LEGACY_SEQUENCER_STEP_COUNT_8 = 8;
const int PRESET_SLOT_COUNT = 8;
const int LEGACY_ENVELOPE_COUNT = 2;
const int LEGACY_MOD_SOURCE_COUNT = 2;
const int LEGACY_MOD_DESTINATION_COUNT = 18;

enum LegacyModDestinationV7 : uint8_t {
  LEGACY_MOD_DEST_AMP_LEVEL,
  LEGACY_MOD_DEST_FILTER_CUTOFF,
  LEGACY_MOD_DEST_FILTER_RESONANCE,
  LEGACY_MOD_DEST_OSC1_PITCH,
  LEGACY_MOD_DEST_OSC2_PITCH,
  LEGACY_MOD_DEST_OSC_MIX,
  LEGACY_MOD_DEST_PWM,
  LEGACY_MOD_DEST_FM_AMOUNT,
  LEGACY_MOD_DEST_WAVEFOLD_AMOUNT,
  LEGACY_MOD_DEST_DRIVE,
  LEGACY_MOD_DEST_PAN,
  LEGACY_MOD_DEST_LFO1_RATE,
  LEGACY_MOD_DEST_LFO1_DEPTH,
  LEGACY_MOD_DEST_LFO2_RATE,
  LEGACY_MOD_DEST_LFO2_DEPTH,
  LEGACY_MOD_DEST_FX_MIX,
  LEGACY_MOD_DEST_DELAY_FEEDBACK,
  LEGACY_MOD_DEST_REVERB_MIX
};

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
  bool oscillatorEnabled[SYNTH_OSCILLATOR_COUNT];
  int oscillatorLevels[SYNTH_OSCILLATOR_COUNT];
  int oscillatorShape[SYNTH_OSCILLATOR_COUNT];
  int oscillatorTune[SYNTH_OSCILLATOR_COUNT];
  int oscillatorFold[SYNTH_OSCILLATOR_COUNT];
  int oscillatorDetune[SYNTH_OSCILLATOR_COUNT];
  int oscillatorOctave[SYNTH_OSCILLATOR_COUNT];
  int oscillatorPhasePercent[SYNTH_OSCILLATOR_COUNT];
  int oscillatorPulseWidthPercent[SYNTH_OSCILLATOR_COUNT];
  int oscillatorPanPercent[SYNTH_OSCILLATOR_COUNT];
  int oscillatorKeyTrackingPercent[SYNTH_OSCILLATOR_COUNT];
  int oscillatorModulationAmountPercent[SYNTH_OSCILLATOR_COUNT];
  bool envelopeEnabled[ENVELOPE_COUNT];
  int envelopeAmountPercent[ENVELOPE_COUNT];
  int envelopeVelocitySensitivityPercent[ENVELOPE_COUNT];
  int envelopeRetriggerMode[ENVELOPE_COUNT];
  LfoControlParameters lfoParameters[SYNTH_LFO_COUNT];
  int filterResonance;
  int synthNoiseLevel;
  int synthPan;
};

// Layout v7 esatto, prima dell'estensione del patch DSP Daisy.
struct LegacySynthPresetV7 {
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
  AdsrParameters envelopeParameters[LEGACY_ENVELOPE_COUNT];
  ModRouteSettings
    modulationRoutes[LEGACY_MOD_SOURCE_COUNT][LEGACY_MOD_DESTINATION_COUNT];
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
  AdsrParameters envelopeParameters[LEGACY_ENVELOPE_COUNT];
  ModRouteSettings
    modulationRoutes[LEGACY_MOD_SOURCE_COUNT][LEGACY_MOD_DESTINATION_COUNT];
};

static_assert(offsetof(LegacySynthPresetV6, envelopeParameters) == sizeof(LegacySynthPresetV4),
              "Preset v5/v6 must retain the complete v4 prefix");
static_assert(offsetof(LegacySynthPresetV7, sequencerStepPitches) ==
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

void initializeModernPresetDefaults(SynthPreset& preset) {
  for (int oscillator = 0; oscillator < SYNTH_OSCILLATOR_COUNT; ++oscillator) {
    preset.oscillatorEnabled[oscillator] = oscillator < WAVE_VOICE_COUNT;
    preset.oscillatorLevels[oscillator] = oscillator < WAVE_VOICE_COUNT ? 100 : 0;
    preset.oscillatorShape[oscillator] = oscillator == 1 ? 1 : 0;
    preset.oscillatorTune[oscillator] = 0;
    preset.oscillatorFold[oscillator] = 0;
    preset.oscillatorDetune[oscillator] = 0;
    preset.oscillatorOctave[oscillator] = 0;
    preset.oscillatorPhasePercent[oscillator] = 0;
    preset.oscillatorPulseWidthPercent[oscillator] = 50;
    preset.oscillatorPanPercent[oscillator] = 0;
    preset.oscillatorKeyTrackingPercent[oscillator] = 100;
    preset.oscillatorModulationAmountPercent[oscillator] = 100;
  }

  for (int envelope = 0; envelope < ENVELOPE_COUNT; ++envelope) {
    preset.envelopeParameters[envelope] = { 10, 80, 68, 320 };
    preset.envelopeEnabled[envelope] = envelope == ENVELOPE_ENV1;
    preset.envelopeAmountPercent[envelope] = 100;
    preset.envelopeVelocitySensitivityPercent[envelope] = 0;
    preset.envelopeRetriggerMode[envelope] = 1;
  }

  for (int source = 0; source < MOD_SOURCE_COUNT; ++source) {
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; ++destination) {
      preset.modulationRoutes[source][destination] = { false, 0 };
    }
  }
  preset.modulationRoutes[MOD_SOURCE_ENV1][MOD_DEST_AMP_LEVEL] = { true, 100 };

  preset.lfoParameters[0] = {
    false, LFO_WAVE_SINE, 100, 0, true, 0, false, true, false, 0,
    MOD_DEST_OSC1_PITCH
  };
  preset.lfoParameters[1] = {
    false, LFO_WAVE_TRIANGLE, 100, 0, true, 0, false, true, false, 0,
    MOD_DEST_FILTER_CUTOFF
  };
  preset.modulationRoutes[MOD_SOURCE_LFO1][MOD_DEST_OSC1_PITCH] = { true, 100 };
  preset.modulationRoutes[MOD_SOURCE_LFO2][MOD_DEST_FILTER_CUTOFF] = { true, 100 };
  preset.filterResonance = 15;
  preset.synthNoiseLevel = 0;
  preset.synthPan = 0;
}

int mapLegacyModDestinationV7(int legacyDestination) {
  switch (legacyDestination) {
    case LEGACY_MOD_DEST_AMP_LEVEL: return MOD_DEST_AMP_LEVEL;
    case LEGACY_MOD_DEST_FILTER_CUTOFF: return MOD_DEST_FILTER_CUTOFF;
    case LEGACY_MOD_DEST_FILTER_RESONANCE: return MOD_DEST_FILTER_RESONANCE;
    case LEGACY_MOD_DEST_OSC1_PITCH: return MOD_DEST_OSC1_PITCH;
    case LEGACY_MOD_DEST_OSC2_PITCH: return MOD_DEST_OSC2_PITCH;
    case LEGACY_MOD_DEST_OSC_MIX: return MOD_DEST_ALL_OSC_PITCH;
    case LEGACY_MOD_DEST_PWM: return MOD_DEST_OSC1_PULSE_WIDTH;
    case LEGACY_MOD_DEST_FM_AMOUNT: return MOD_DEST_ALL_OSC_PITCH;
    case LEGACY_MOD_DEST_WAVEFOLD_AMOUNT: return MOD_DEST_OSC1_WAVEFOLD;
    case LEGACY_MOD_DEST_PAN: return MOD_DEST_PAN;
    case LEGACY_MOD_DEST_FX_MIX: return MOD_DEST_DELAY_MIX;
    case LEGACY_MOD_DEST_DELAY_FEEDBACK: return MOD_DEST_DELAY_MIX;
    case LEGACY_MOD_DEST_REVERB_MIX: return MOD_DEST_REVERB_MIX;
    default: return -1;
  }
}

void migrateLegacyPresetV7(const LegacySynthPresetV7& legacy, SynthPreset& migrated) {
  migrated = {};
  memcpy(&migrated, &legacy, offsetof(LegacySynthPresetV7, envelopeParameters));
  migrated.version = PRESET_VERSION;
  initializeModernPresetDefaults(migrated);

  memcpy(migrated.envelopeParameters,
         legacy.envelopeParameters,
         sizeof(legacy.envelopeParameters));
  for (int source = 0; source < LEGACY_MOD_SOURCE_COUNT; ++source) {
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; ++destination) {
      migrated.modulationRoutes[source][destination] = { false, 0 };
    }

    for (int legacyDestination = 0;
         legacyDestination < LEGACY_MOD_DESTINATION_COUNT;
         ++legacyDestination) {
      const ModRouteSettings& legacyRoute =
        legacy.modulationRoutes[source][legacyDestination];
      int destination = mapLegacyModDestinationV7(legacyDestination);
      if (!legacyRoute.enabled || destination < 0) continue;

      ModRouteSettings& route = migrated.modulationRoutes[source][destination];
      route.enabled = true;
      route.amountPercent = (int8_t)clampModulationAmount(
        (int)route.amountPercent + (int)legacyRoute.amountPercent
      );
    }
  }

  for (int oscillator = 0; oscillator < WAVE_VOICE_COUNT; ++oscillator) {
    migrated.oscillatorLevels[oscillator] = legacy.waveVoiceLevels[oscillator];
    migrated.oscillatorShape[oscillator] = legacy.waveVoiceShape[oscillator];
    migrated.oscillatorTune[oscillator] = legacy.waveVoiceTune[oscillator];
    migrated.oscillatorFold[oscillator] = legacy.waveVoiceFold[oscillator];
    migrated.oscillatorDetune[oscillator] = legacy.waveVoiceDetune[oscillator];
  }
}

void migrateLegacyPresetV6(const LegacySynthPresetV6& legacy,
                           LegacySynthPresetV7& migrated) {
  migrated = {};
  memcpy(&migrated, &legacy, offsetof(LegacySynthPresetV7, sequencerStepPitches));
  migrated.version = LEGACY_PRESET_VERSION_7;
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
  memcpy(migrated.envelopeParameters,
         legacy.envelopeParameters,
         sizeof(legacy.envelopeParameters));
  memcpy(migrated.modulationRoutes,
         legacy.modulationRoutes,
         sizeof(legacy.modulationRoutes));
}

void capturePreset(SynthPreset& preset) {
  preset = {};
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
  memcpy(preset.waveVoiceLevels, waveVoiceLevels, sizeof(preset.waveVoiceLevels));
  memcpy(preset.waveVoiceShape, waveVoiceShape, sizeof(preset.waveVoiceShape));
  memcpy(preset.waveVoiceTune, waveVoiceTune, sizeof(preset.waveVoiceTune));
  memcpy(preset.waveVoiceFold, waveVoiceFold, sizeof(preset.waveVoiceFold));
  memcpy(preset.waveVoiceDetune, waveVoiceDetune, sizeof(preset.waveVoiceDetune));
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
  memcpy(preset.oscillatorEnabled, oscillatorEnabled, sizeof(oscillatorEnabled));
  memcpy(preset.oscillatorLevels, waveVoiceLevels, sizeof(waveVoiceLevels));
  memcpy(preset.oscillatorShape, waveVoiceShape, sizeof(waveVoiceShape));
  memcpy(preset.oscillatorTune, waveVoiceTune, sizeof(waveVoiceTune));
  memcpy(preset.oscillatorFold, waveVoiceFold, sizeof(waveVoiceFold));
  memcpy(preset.oscillatorDetune, waveVoiceDetune, sizeof(waveVoiceDetune));
  memcpy(preset.oscillatorOctave, oscillatorOctave, sizeof(oscillatorOctave));
  memcpy(preset.oscillatorPhasePercent,
         oscillatorPhasePercent,
         sizeof(oscillatorPhasePercent));
  memcpy(preset.oscillatorPulseWidthPercent,
         oscillatorPulseWidthPercent,
         sizeof(oscillatorPulseWidthPercent));
  memcpy(preset.oscillatorPanPercent,
         oscillatorPanPercent,
         sizeof(oscillatorPanPercent));
  memcpy(preset.oscillatorKeyTrackingPercent,
         oscillatorKeyTrackingPercent,
         sizeof(oscillatorKeyTrackingPercent));
  memcpy(preset.oscillatorModulationAmountPercent,
         oscillatorModulationAmountPercent,
         sizeof(oscillatorModulationAmountPercent));
  memcpy(preset.envelopeEnabled, envelopeEnabled, sizeof(envelopeEnabled));
  memcpy(preset.envelopeAmountPercent,
         envelopeAmountPercent,
         sizeof(envelopeAmountPercent));
  memcpy(preset.envelopeVelocitySensitivityPercent,
         envelopeVelocitySensitivityPercent,
         sizeof(envelopeVelocitySensitivityPercent));
  memcpy(preset.envelopeRetriggerMode,
         envelopeRetriggerMode,
         sizeof(envelopeRetriggerMode));
  memcpy(preset.lfoParameters, lfoParameters, sizeof(lfoParameters));
  preset.filterResonance = filterResonance;
  preset.synthNoiseLevel = synthNoiseLevel;
  preset.synthPan = synthPan;
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
  memcpy(waveVoiceLevels, preset.oscillatorLevels, sizeof(waveVoiceLevels));
  memcpy(waveVoiceShape, preset.oscillatorShape, sizeof(waveVoiceShape));
  memcpy(waveVoiceTune, preset.oscillatorTune, sizeof(waveVoiceTune));
  memcpy(waveVoiceFold, preset.oscillatorFold, sizeof(waveVoiceFold));
  memcpy(waveVoiceDetune, preset.oscillatorDetune, sizeof(waveVoiceDetune));
  memcpy(oscillatorEnabled, preset.oscillatorEnabled, sizeof(oscillatorEnabled));
  memcpy(oscillatorOctave, preset.oscillatorOctave, sizeof(oscillatorOctave));
  memcpy(oscillatorPhasePercent,
         preset.oscillatorPhasePercent,
         sizeof(oscillatorPhasePercent));
  memcpy(oscillatorPulseWidthPercent,
         preset.oscillatorPulseWidthPercent,
         sizeof(oscillatorPulseWidthPercent));
  memcpy(oscillatorPanPercent,
         preset.oscillatorPanPercent,
         sizeof(oscillatorPanPercent));
  memcpy(oscillatorKeyTrackingPercent,
         preset.oscillatorKeyTrackingPercent,
         sizeof(oscillatorKeyTrackingPercent));
  memcpy(oscillatorModulationAmountPercent,
         preset.oscillatorModulationAmountPercent,
         sizeof(oscillatorModulationAmountPercent));
  for (int oscillator = 0; oscillator < SYNTH_OSCILLATOR_COUNT; oscillator++) {
    waveVoiceLevels[oscillator] = clampPresetInt(waveVoiceLevels[oscillator], 0, 100);
    waveVoiceShape[oscillator] = clampPresetInt(waveVoiceShape[oscillator], 0, WAVE_SHAPE_COUNT - 1);
    waveVoiceTune[oscillator] = clampPresetInt(waveVoiceTune[oscillator], -48, 48);
    waveVoiceFold[oscillator] = clampPresetInt(waveVoiceFold[oscillator], 0, 100);
    waveVoiceDetune[oscillator] = clampPresetInt(waveVoiceDetune[oscillator], -100, 100);
    oscillatorOctave[oscillator] = clampPresetInt(oscillatorOctave[oscillator], -4, 4);
    oscillatorPhasePercent[oscillator] = clampPresetInt(oscillatorPhasePercent[oscillator], 0, 100);
    oscillatorPulseWidthPercent[oscillator] = clampPresetInt(
      oscillatorPulseWidthPercent[oscillator], 1, 99
    );
    oscillatorPanPercent[oscillator] = clampPresetInt(oscillatorPanPercent[oscillator], -100, 100);
    oscillatorKeyTrackingPercent[oscillator] = clampPresetInt(
      oscillatorKeyTrackingPercent[oscillator], 0, 100
    );
    oscillatorModulationAmountPercent[oscillator] = clampPresetInt(
      oscillatorModulationAmountPercent[oscillator], -100, 100
    );
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
  memcpy(envelopeEnabled, preset.envelopeEnabled, sizeof(envelopeEnabled));
  memcpy(envelopeAmountPercent,
         preset.envelopeAmountPercent,
         sizeof(envelopeAmountPercent));
  memcpy(envelopeVelocitySensitivityPercent,
         preset.envelopeVelocitySensitivityPercent,
         sizeof(envelopeVelocitySensitivityPercent));
  memcpy(envelopeRetriggerMode,
         preset.envelopeRetriggerMode,
         sizeof(envelopeRetriggerMode));
  memcpy(lfoParameters, preset.lfoParameters, sizeof(lfoParameters));
  filterResonance = clampPresetInt(preset.filterResonance, 0, 100);
  synthNoiseLevel = clampPresetInt(preset.synthNoiseLevel, 0, 100);
  synthPan = clampPresetInt(preset.synthPan, -100, 100);
  for (int envelope = 0; envelope < ENVELOPE_COUNT; envelope++) {
    envelopeParameters[envelope].attackMs = clampPresetInt(envelopeParameters[envelope].attackMs, 0, 500);
    envelopeParameters[envelope].decayMs = clampPresetInt(envelopeParameters[envelope].decayMs, 0, 2000);
    envelopeParameters[envelope].sustainPercent = clampPresetInt(envelopeParameters[envelope].sustainPercent, 0, 100);
    envelopeParameters[envelope].releaseMs = clampPresetInt(envelopeParameters[envelope].releaseMs, 0, 3000);
    envelopeAmountPercent[envelope] = clampPresetInt(envelopeAmountPercent[envelope], -100, 100);
    envelopeVelocitySensitivityPercent[envelope] = clampPresetInt(
      envelopeVelocitySensitivityPercent[envelope], 0, 100
    );
    envelopeRetriggerMode[envelope] = clampPresetInt(envelopeRetriggerMode[envelope], 0, 2);
  }
  for (int source = 0; source < MOD_SOURCE_COUNT; source++) {
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; destination++) {
      modulationRoutes[source][destination].amountPercent = (int8_t)clampModulationAmount(
        (int)modulationRoutes[source][destination].amountPercent
      );
    }
  }
  for (int lfo = 0; lfo < SYNTH_LFO_COUNT; ++lfo) {
    lfoParameters[lfo].waveform = (uint8_t)clampPresetInt(
      lfoParameters[lfo].waveform, 0, LFO_WAVE_COUNT - 1
    );
    lfoParameters[lfo].frequencyCentiHz = clampPresetInt(
      lfoParameters[lfo].frequencyCentiHz, 0, 20000
    );
    lfoParameters[lfo].phasePercent = clampPresetInt(lfoParameters[lfo].phasePercent, 0, 100);
    lfoParameters[lfo].amountPercent = clampPresetInt(lfoParameters[lfo].amountPercent, -100, 100);
    lfoParameters[lfo].performanceDestination = (uint8_t)clampPresetInt(
      lfoParameters[lfo].performanceDestination, 0, MOD_DESTINATION_COUNT - 1
    );
  }
  clampSequencerIndexes();
  resetSequencerClock();
}

void applyLegacyPresetV7(const LegacySynthPresetV7& legacy) {
  SynthPreset migrated;
  migrateLegacyPresetV7(legacy, migrated);
  applyPreset(migrated);
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

  LegacySynthPresetV7 migrated;
  migrateLegacyPresetV6(expanded, migrated);
  applyLegacyPresetV7(migrated);
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
  memcpy(waveVoiceLevels, preset.waveVoiceLevels, sizeof(preset.waveVoiceLevels));
  memcpy(waveVoiceShape, preset.waveVoiceShape, sizeof(preset.waveVoiceShape));
  memcpy(waveVoiceTune, preset.waveVoiceTune, sizeof(preset.waveVoiceTune));
  memcpy(waveVoiceFold, preset.waveVoiceFold, sizeof(preset.waveVoiceFold));
  memcpy(waveVoiceDetune, preset.waveVoiceDetune, sizeof(preset.waveVoiceDetune));
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
  envelopeParameters[ENVELOPE_ENV3] = { 10, 80, 68, 320 };
  oscillatorEnabled[0] = true;
  oscillatorEnabled[1] = true;
  oscillatorEnabled[2] = false;
  waveVoiceLevels[2] = 0;
  waveVoiceShape[2] = 0;
  waveVoiceTune[2] = 0;
  waveVoiceFold[2] = 0;
  waveVoiceDetune[2] = 0;
  for (int oscillator = 0; oscillator < SYNTH_OSCILLATOR_COUNT; ++oscillator) {
    oscillatorOctave[oscillator] = 0;
    oscillatorPhasePercent[oscillator] = 0;
    oscillatorPulseWidthPercent[oscillator] = 50;
    oscillatorPanPercent[oscillator] = 0;
    oscillatorKeyTrackingPercent[oscillator] = 100;
    oscillatorModulationAmountPercent[oscillator] = 100;
  }
  envelopeEnabled[0] = true;
  envelopeEnabled[1] = false;
  envelopeEnabled[2] = false;
  for (int envelope = 0; envelope < ENVELOPE_COUNT; ++envelope) {
    envelopeAmountPercent[envelope] = 100;
    envelopeVelocitySensitivityPercent[envelope] = 0;
    envelopeRetriggerMode[envelope] = 1;
  }
  lfoParameters[0] = {
    false, LFO_WAVE_SINE, 100, 0, true, 0, false, true, false, 0,
    MOD_DEST_OSC1_PITCH
  };
  lfoParameters[1] = {
    false, LFO_WAVE_TRIANGLE, 100, 0, true, 0, false, true, false, 0,
    MOD_DEST_FILTER_CUTOFF
  };
  filterResonance = 15;
  synthNoiseLevel = 0;
  synthPan = 0;
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

  if (storedSize == sizeof(LegacySynthPresetV7)) {
    LegacySynthPresetV7 preset;
    size_t read = preferences.getBytes(key, &preset, sizeof(preset));
    preferences.end();
    if (read != sizeof(preset) || preset.magic != PRESET_MAGIC ||
        preset.version != LEGACY_PRESET_VERSION_7) return false;
    applyLegacyPresetV7(preset);
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
    LegacySynthPresetV7 migrated;
    migrateLegacyPresetV6(preset, migrated);
    applyLegacyPresetV7(migrated);
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
