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
  return action == ACTION_VOICE_WAVE ||
         action == ACTION_VOICE_TUNE ||
         action == ACTION_VOICE_FOLD ||
         action == ACTION_VOICE_DETUNE ||
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

  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    checksum = checksum * 31 + waveVoiceShape[voice];
    checksum = checksum * 31 + waveVoiceTune[voice];
    checksum = checksum * 31 + waveVoiceFold[voice];
    checksum = checksum * 31 + waveVoiceDetune[voice];
  }

  return checksum;
}

int getKickParamChecksum() {
  int checksum = kickPitch;
  checksum = checksum * 31 + kickDecay;
  checksum = checksum * 31 + kickSweep;
  checksum = checksum * 31 + kickClick;
  checksum = checksum * 31 + kickDrive;
  return checksum;
}

void resetSequencerClock() {
  clampSequencerIndexes();
  sequencerCurrentStep = sequencerSelectedStep;
}
