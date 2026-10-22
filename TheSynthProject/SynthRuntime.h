#pragma once

// Runtime helpers shared by the ESP32 UI and the Daisy control bridge. No
// samples are rendered here: the Daisy Seed is the only audio engine.

float getOscillatorSample(float phase) {
  // Used only by TFT meters and waveform previews.
  return getOscillatorSampleForParams(
    phase,
    waveform,
    (float)waveFold,
    (float)wavePhase
  );
}

bool isKickTrackAudible() {
  return kickLevel > 0;
}

bool isAudioTestActive() {
  return uiMode == MODE_EDIT && getSelectedAction() == ACTION_AUDIO_TEST;
}

bool isVoicePreviewActive() {
  if (uiMode != MODE_EDIT) return false;

  MenuAction action = getSelectedAction();
  return action == ACTION_OSC_ENABLED ||
         action == ACTION_VOICE_WAVE ||
         action == ACTION_OSC_LEVEL ||
         action == ACTION_OSC_OCTAVE ||
         action == ACTION_VOICE_TUNE ||
         action == ACTION_VOICE_DETUNE ||
         action == ACTION_OSC_PHASE ||
         action == ACTION_OSC_PULSE_WIDTH ||
         action == ACTION_VOICE_FOLD ||
         action == ACTION_OSC_PAN ||
         action == ACTION_OSC_KEY_TRACK ||
         action == ACTION_OSC_MOD_AMOUNT ||
         action == ACTION_VOICE_SCOPE;
}

void triggerKickVoice() {
  // DaisyBridge sends this monotonically increasing value. A state snapshot
  // can therefore carry a one-shot kick without adding a second message type.
  kickTriggerSequence++;
}

int getSequencerNoteChecksum() {
  int checksum = getActiveSequencerVoice();

  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      checksum = checksum * 31 + sequencerStepPitches[voice][step];
      checksum = checksum * 3 + (sequencerStepEnabled[voice][step] ? 1 : 0);
    }
  }

  return checksum;
}

int getWaveVoiceParamChecksum() {
  int checksum = 0;

  for (const OscillatorBank& bank : voiceOscillators) {
    checksum = checksum * 31 + bank.masterLevelPercent;
    for (int oscillator = 0; oscillator < SYNTH_OSCILLATOR_COUNT; oscillator++) {
      checksum = checksum * 31 + (bank.oscillatorEnabled[oscillator] ? 1 : 0);
      checksum = checksum * 31 + bank.waveVoiceShape[oscillator];
      checksum = checksum * 31 + bank.waveVoiceLevels[oscillator];
      checksum = checksum * 31 + bank.oscillatorOctave[oscillator];
      checksum = checksum * 31 + bank.waveVoiceTune[oscillator];
      checksum = checksum * 31 + bank.waveVoiceFold[oscillator];
      checksum = checksum * 31 + bank.waveVoiceDetune[oscillator];
      checksum = checksum * 31 + bank.oscillatorPhasePercent[oscillator];
      checksum = checksum * 31 + bank.oscillatorPulseWidthPercent[oscillator];
      checksum = checksum * 31 + bank.oscillatorPanPercent[oscillator];
      checksum = checksum * 31 + bank.oscillatorKeyTrackingPercent[oscillator];
      checksum = checksum * 31 + bank.oscillatorModulationAmountPercent[oscillator];
    }
  }

  return checksum;
}

int getKickParamChecksum() {
  int checksum = kickPitch;
  checksum = checksum * 31 + kickDecay;
  checksum = checksum * 31 + kickSweep;
  checksum = checksum * 31 + kickAttack;
  checksum = checksum * 31 + kickDrive;
  return checksum;
}

void resetSequencerClock() {
  clampSequencerIndexes();
  // Playback always starts from step 1. Editing selection is deliberately
  // independent and can never move or stop the playhead.
  sequencerCurrentStep = 0;
}
