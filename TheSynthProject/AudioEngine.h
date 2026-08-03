#pragma once

float applyWaveFold(float sample) {
  return applyWaveFoldAmount(sample, (float)waveFold);
}

float getOscillatorSample(float phase) {
  return getOscillatorSampleForParams(phase, waveform, (float)waveFold, (float)wavePhase);
}

// Values below are owned by the audio task. UI-facing parameters are sampled
// only at block boundaries and never read repeatedly inside the sample loop.
static float audioVoicePhaseSteps[WAVE_VOICE_COUNT] = { 0.0f, 0.0f };
static float audioFilterCoefficient = 0.1f;
static float audioDelayMixAmount = 0.0f;
static float audioDelayFeedbackAmount = 0.35f;
static float audioDelayTargetSamples = (280.0f * SYNTH_AUDIO_SAMPLE_RATE) / 1000.0f;
static float audioDelayCurrentSamples = (280.0f * SYNTH_AUDIO_SAMPLE_RATE) / 1000.0f;
static float audioReverbMixAmount = 0.0f;
static float audioReverbRoomAmount = 0.65f;
static uint32_t audioAttackSamples = (10U * SYNTH_AUDIO_SAMPLE_RATE) / 1000U;
static uint32_t audioDecaySamples = (80U * SYNTH_AUDIO_SAMPLE_RATE) / 1000U;
static uint32_t audioReleaseSamples = (320U * SYNTH_AUDIO_SAMPLE_RATE) / 1000U;
static uint32_t audioKickMaxSamples = (420U * SYNTH_AUDIO_SAMPLE_RATE) / 1000U;
static uint32_t audioSequencerStepSamples = (SYNTH_AUDIO_SAMPLE_RATE * 15U) / 120U;
static uint32_t audioSequencerGateSamples = ((SYNTH_AUDIO_SAMPLE_RATE * 15U) / 120U) * 70U / 100U;
static float audioSustainLevel = 0.68f;
static bool audioDelayRequested = false;
static bool audioReverbRequested = false;
static bool audioDelayPathActive = false;
static bool audioReverbPathActive = false;

bool isVoicePreviewActive();

void clearSynthFilterState() {
  filterLowState = 0.0f;
  filterBandState = 0.0f;
}

void clearSynthDelayState() {
  memset(delayBuffer, 0, sizeof(delayBuffer));
  delayWriteIndex = 0;
}

void clearSynthReverbState() {
  memset(reverbBufferA, 0, sizeof(reverbBufferA));
  memset(reverbBufferB, 0, sizeof(reverbBufferB));
  memset(reverbBufferC, 0, sizeof(reverbBufferC));
  reverbIndexA = 0;
  reverbIndexB = 0;
  reverbIndexC = 0;
}

void resetSynthDspState(bool clearEffectBuffers) {
  clearSynthFilterState();
  synthEnvelopeSmoothed = 0.0f;

  if (clearEffectBuffers) {
    clearSynthDelayState();
    clearSynthReverbState();
    audioDelayPathActive = false;
    audioReverbPathActive = false;
  }
}

float getCrossfadedVoiceSample(int voiceIndex, float phase, int targetShape) {
  targetShape = clampInt(targetShape, 0, WAVE_SHAPE_COUNT - 1);

  if (targetShape != synthRenderedVoiceShape[voiceIndex]) {
    synthPreviousVoiceShape[voiceIndex] = synthRenderedVoiceShape[voiceIndex];
    synthRenderedVoiceShape[voiceIndex] = targetShape;
    synthVoiceShapeCrossfadeRemaining[voiceIndex] = SYNTH_SHAPE_CROSSFADE_SAMPLES;
  }

  float currentSample = getOscillatorSampleForParams(
    phase,
    synthRenderedVoiceShape[voiceIndex],
    audioWaveVoiceFold[voiceIndex],
    0.0f
  );

  int remaining = synthVoiceShapeCrossfadeRemaining[voiceIndex];
  if (remaining <= 0) return currentSample;

  float previousSample = getOscillatorSampleForParams(
    phase,
    synthPreviousVoiceShape[voiceIndex],
    audioWaveVoiceFold[voiceIndex],
    0.0f
  );
  float mix = 1.0f - ((float)remaining / (float)SYNTH_SHAPE_CROSSFADE_SAMPLES);
  synthVoiceShapeCrossfadeRemaining[voiceIndex] = remaining - 1;

  return previousSample + (currentSample - previousSample) * mix;
}

void updateAudioVoicePhaseSteps() {
  int step = clampInt(sequencerCurrentStep, 0, SEQUENCER_STEP_COUNT - 1);

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    float semitoneOffset = audioWaveVoiceTune[i] + audioWaveVoiceDetune[i] / 100.0f;
    if (sequencerEnabled) semitoneOffset += getSequencerStepNote(i, step);

    float tuneRatio = powf(2.0f, semitoneOffset / 12.0f);
    audioVoicePhaseSteps[i] = (audioFrequency * tuneRatio) / (float)SYNTH_AUDIO_SAMPLE_RATE;
  }
}

void updateAudioParametersForBlock() {
  const float amount = SYNTH_PARAMETER_SMOOTH_BLOCK;
  int targetFrequency = clampInt(frequency, 20, 2000);
  int targetVolume = clampInt(volume, 0, 100);

  audioFrequency = smoothToward(audioFrequency, (float)targetFrequency, amount);
  audioVolume = smoothToward(audioVolume, (float)targetVolume, amount);
  audioWaveFold = smoothToward(audioWaveFold, (float)clampInt(waveFold, 0, 100), amount);
  audioWavePhase = smoothAngleDegrees(audioWavePhase, (float)wrapIndex(wavePhase, 360), amount);
  audioKickLevel = smoothToward(audioKickLevel, (float)clampInt(kickLevel, 0, 100), amount);
  audioKickPitch = smoothToward(audioKickPitch, (float)clampInt(kickPitch, 25, 120), amount);
  audioKickDecay = smoothToward(audioKickDecay, (float)clampInt(kickDecay, 80, 1200), amount);
  audioKickSweep = smoothToward(audioKickSweep, (float)clampInt(kickSweep, 0, 260), amount);
  audioKickClick = smoothToward(audioKickClick, (float)clampInt(kickClick, 0, 100), amount);
  audioKickDrive = smoothToward(audioKickDrive, (float)clampInt(kickDrive, 0, 100), amount);
  audioFilterAmount = smoothToward(audioFilterAmount, filterEnabled ? 1.0f : 0.0f, amount);
  audioFilterCutoff = smoothToward(
    audioFilterCutoff,
    (float)clampInt(filterCutoff, FILTER_MIN_CUTOFF_HZ, FILTER_MAX_CUTOFF_HZ),
    amount
  );
  audioFilterType = clampInt(filterType, 0, FILTER_TYPE_COUNT - 1);

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    audioWaveVoiceLevels[i] = smoothToward(audioWaveVoiceLevels[i], (float)clampInt(waveVoiceLevels[i], 0, 100), amount);
    audioWaveVoiceShape[i] = clampInt(waveVoiceShape[i], 0, WAVE_SHAPE_COUNT - 1);
    audioWaveVoiceTune[i] = smoothToward(audioWaveVoiceTune[i], (float)clampInt(waveVoiceTune[i], -24, 24), amount);
    audioWaveVoiceFold[i] = smoothToward(audioWaveVoiceFold[i], (float)clampInt(waveVoiceFold[i], 0, 100), amount);
    audioWaveVoiceDetune[i] = smoothToward(audioWaveVoiceDetune[i], (float)clampInt(waveVoiceDetune[i], -50, 50), amount);
  }

  float cutoffRadians = TWO_PI_F * audioFilterCutoff / (float)SYNTH_AUDIO_SAMPLE_RATE;
  audioFilterCoefficient = clampFloat(1.0f - expf(-cutoffRadians), 0.001f, 0.95f);

  int targetDelayMix = clampInt(delayMix, 0, 100);
  int targetReverbMix = clampInt(reverbMix, 0, 100);
  audioDelayMixAmount = smoothToward(audioDelayMixAmount, targetDelayMix / 100.0f, amount);
  audioDelayFeedbackAmount = smoothToward(audioDelayFeedbackAmount, clampInt(delayFeedback, 0, 90) / 100.0f, amount);
  audioDelayTargetSamples = (float)((clampInt(delayTimeMs, 20, DELAY_MAX_MS) * SYNTH_AUDIO_SAMPLE_RATE) / 1000);
  audioReverbMixAmount = smoothToward(audioReverbMixAmount, targetReverbMix / 100.0f, amount);
  audioReverbRoomAmount = smoothToward(audioReverbRoomAmount, clampInt(reverbRoom, 0, 95) / 100.0f, amount);
  audioDelayRequested = targetDelayMix > 0 || audioDelayMixAmount > 0.001f;
  audioReverbRequested = targetReverbMix > 0 || audioReverbMixAmount > 0.001f;

  audioAttackSamples = ((uint32_t)clampInt(attack, 0, 500) * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
  audioDecaySamples = ((uint32_t)clampInt(decay, 0, 2000) * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
  audioReleaseSamples = ((uint32_t)clampInt(releaseMs, 0, 3000) * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
  audioSustainLevel = clampInt(sustain, 0, 100) / 100.0f;
  audioKickMaxSamples = (uint32_t)((audioKickDecay * (float)SYNTH_AUDIO_SAMPLE_RATE) / 1000.0f);
  if (audioKickMaxSamples < 80) audioKickMaxSamples = 80;

  int safeBpm = clampInt(sequencerBpm, 40, 240);
  audioSequencerStepSamples = (uint32_t)max(1, (SYNTH_AUDIO_SAMPLE_RATE * 15) / safeBpm);
  audioSequencerGateSamples = (audioSequencerStepSamples * 70U) / 100U;
  if (audioSequencerGateSamples < 1) audioSequencerGateSamples = 1;

  updateAudioVoicePhaseSteps();
}

float getAudioWaveMixSample() {
  float mixed = 0.0f;
  float totalGain = 0.0f;
  int step = clampInt(sequencerCurrentStep, 0, SEQUENCER_STEP_COUNT - 1);
  int previewVoice = isVoicePreviewActive() ? getCurrentWaveVoiceIndex() : -1;

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    if (!sequencerEnabled && previewVoice >= 0 && i != previewVoice) continue;

    float gain = audioWaveVoiceLevels[i] / 100.0f;
    if (gain <= 0.001f) continue;

    if (sequencerEnabled) {
      if (!isSequencerStepEnabled(i, step)) continue;
    }

    mixed += getCrossfadedVoiceSample(i, synthVoicePhases[i], audioWaveVoiceShape[i]) * gain;
    totalGain += gain;

    synthVoicePhases[i] += audioVoicePhaseSteps[i];
    if (synthVoicePhases[i] >= 1.0f) {
      synthVoicePhases[i] -= 1.0f;
    }
  }

  if (totalGain <= 0.001f) return 0.0f;

  float normalizer = totalGain < 1.0f ? 1.0f : totalGain;
  return mixed / normalizer;
}

float applySynthFilter(float input) {
  if (audioFilterAmount <= 0.001f) {
    clearSynthFilterState();
    return input;
  }

  // Two bounded one-pole states are cheaper and cannot run away at high
  // cutoff like the previous state-variable filter could.
  filterLowState += audioFilterCoefficient * (input - filterLowState);
  float high = input - filterLowState;
  filterBandState += audioFilterCoefficient * (high - filterBandState);

  float filtered = filterLowState;
  if (audioFilterType == 1) {
    filtered = high;
  } else if (audioFilterType == 2) {
    filtered = filterBandState;
  }

  if (!isfinite(filtered)) {
    clearSynthFilterState();
    synthAudioDspRecoveries = synthAudioDspRecoveries + 1;
    return input;
  }

  return input + (filtered - input) * audioFilterAmount;
}

float processSynthDelay(float input) {
  if (!audioDelayRequested) {
    if (audioDelayPathActive) clearSynthDelayState();
    audioDelayPathActive = false;
    return input;
  }

  if (!audioDelayPathActive) {
    clearSynthDelayState();
    audioDelayCurrentSamples = audioDelayTargetSamples;
    audioDelayPathActive = true;
  }

  // Slew plus linear interpolation prevents a moving delay tap from jumping
  // between unrelated samples when the encoder changes the delay time.
  audioDelayCurrentSamples = smoothToward(audioDelayCurrentSamples, audioDelayTargetSamples, 0.00035f);
  audioDelayCurrentSamples = clampFloat(audioDelayCurrentSamples, 1.0f, (float)(DELAY_BUFFER_SAMPLES - 2));

  float readPosition = (float)delayWriteIndex - audioDelayCurrentSamples;
  if (readPosition < 0.0f) readPosition += (float)DELAY_BUFFER_SAMPLES;
  uint32_t firstIndex = (uint32_t)readPosition;
  uint32_t secondIndex = firstIndex + 1;
  if (secondIndex >= DELAY_BUFFER_SAMPLES) secondIndex = 0;
  float fraction = readPosition - (float)firstIndex;
  float first = delayBuffer[firstIndex] / 32767.0f;
  float second = delayBuffer[secondIndex] / 32767.0f;
  float delayed = first + (second - first) * fraction;
  float delayStored = input + delayed * audioDelayFeedbackAmount;
  delayBuffer[delayWriteIndex] = (int16_t)(clampFloat(delayStored, -1.0f, 1.0f) * 32767.0f);
  delayWriteIndex++;
  if (delayWriteIndex >= DELAY_BUFFER_SAMPLES) delayWriteIndex = 0;

  return input + (delayed - input) * audioDelayMixAmount;
}

float processSynthReverb(float input) {
  if (!audioReverbRequested) {
    if (audioReverbPathActive) clearSynthReverbState();
    audioReverbPathActive = false;
    return input;
  }

  if (!audioReverbPathActive) {
    clearSynthReverbState();
    audioReverbPathActive = true;
  }

  float combA = reverbBufferA[reverbIndexA] / 32767.0f;
  float combB = reverbBufferB[reverbIndexB] / 32767.0f;
  float combC = reverbBufferC[reverbIndexC] / 32767.0f;
  reverbBufferA[reverbIndexA] = (int16_t)(clampFloat(input + combA * audioReverbRoomAmount, -1.0f, 1.0f) * 32767.0f);
  reverbBufferB[reverbIndexB] = (int16_t)(clampFloat(input + combB * audioReverbRoomAmount * 0.96f, -1.0f, 1.0f) * 32767.0f);
  reverbBufferC[reverbIndexC] = (int16_t)(clampFloat(input + combC * audioReverbRoomAmount * 0.92f, -1.0f, 1.0f) * 32767.0f);
  if (++reverbIndexA >= REVERB_BUFFER_A_SAMPLES) reverbIndexA = 0;
  if (++reverbIndexB >= REVERB_BUFFER_B_SAMPLES) reverbIndexB = 0;
  if (++reverbIndexC >= REVERB_BUFFER_C_SAMPLES) reverbIndexC = 0;

  float reverbWet = (combA + combB + combC) / 3.0f;
  return input + (reverbWet - input) * audioReverbMixAmount;
}

float processDelayAndReverb(float input) {
  if (!isfinite(input)) {
    resetSynthDspState(true);
    synthAudioDspRecoveries = synthAudioDspRecoveries + 1;
    return 0.0f;
  }

  return processSynthReverb(processSynthDelay(input));
}

float getSynthEnvelopeLevel() {
  float level = audioSustainLevel;

  if (audioAttackSamples > 0 && synthEnvelopeSample < audioAttackSamples) {
    level = (float)synthEnvelopeSample / (float)audioAttackSamples;
  } else if (audioDecaySamples > 0 && synthEnvelopeSample < audioAttackSamples + audioDecaySamples) {
    float t = (float)(synthEnvelopeSample - audioAttackSamples) / (float)audioDecaySamples;
    level = 1.0f - (1.0f - audioSustainLevel) * clampFloat(t, 0.0f, 1.0f);
  }

  synthEnvelopeSample++;
  return clampFloat(level, 0.0f, 1.0f);
}

float getDeclickedEnvelopeLevel(float targetLevel) {
  targetLevel = clampFloat(targetLevel, 0.0f, 1.0f);
  synthEnvelopeSmoothed = smoothToward(synthEnvelopeSmoothed, targetLevel, SYNTH_DECLICK_AMOUNT);

  if (targetLevel <= 0.0001f && synthEnvelopeSmoothed <= 0.0001f) {
    synthEnvelopeSmoothed = 0.0f;
  }

  return synthEnvelopeSmoothed;
}

void resetSynthVoice() {
  synthEnvelopeSample = 0;
  synthRetriggerRequested = false;
}

bool isMixerLayerEnabled() {
  return kickLevel > 0;
}

bool isMixerLayerAudible() {
  return isMixerLayerEnabled() || audioKickLevel > 0.05f || kickVoiceActive;
}

bool isPatternClockActive() {
  return transportRunning && (sequencerEnabled || isMixerLayerAudible());
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
  kickPhase = 0.0f;
  kickSampleAge = 0;
  kickVoiceActive = kickLevel > 0;
}

int getSequencerStepSamples() {
  return (int)audioSequencerStepSamples;
}

int getSequencerGateSamples() {
  return (int)audioSequencerGateSamples;
}

int getSequencerNoteChecksum() {
  int checksum = getActiveSequencerVoice();

  for (int voice = 0; voice < WAVE_VOICE_COUNT; voice++) {
    for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
      checksum = checksum * 31 + sequencerStepNotes[voice][step];
      checksum = checksum * 3 + (sequencerStepEnabled[voice][step] ? 1 : 0);
    }
  }

  return checksum;
}

int getWaveVoiceParamChecksum() {
  int checksum = 0;

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    checksum = checksum * 31 + waveVoiceShape[i];
    checksum = checksum * 31 + waveVoiceTune[i];
    checksum = checksum * 31 + waveVoiceFold[i];
    checksum = checksum * 31 + waveVoiceDetune[i];
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
  sequencerSampleCounter = 0;
}

void tickSequencerClock() {
  if (!isPatternClockActive()) return;

  sequencerSampleCounter++;

  if (sequencerSampleCounter >= audioSequencerStepSamples) {
    int length = getSequencerLength();

    sequencerSampleCounter = 0;

    sequencerCurrentStep = (sequencerCurrentStep + 1) % length;
    updateAudioVoicePhaseSteps();

    if (kickLevel > 0 && isKickStepEnabled(sequencerCurrentStep)) {
      triggerKickVoice();
    }

    resetSynthVoice();
  }
}

float getSequencerGateLevel() {
  if (!sequencerEnabled) return (isAudioTestActive() || isVoicePreviewActive()) ? 1.0f : 0.0f;

  int step = sequencerCurrentStep;
  if (step < 0 || step >= SEQUENCER_STEP_COUNT) return 0.0f;
  if (!isAnySequencerVoiceEnabledAtStep(step)) return 0.0f;

  if (sequencerSampleCounter < audioSequencerGateSamples) return 1.0f;

  if (audioReleaseSamples == 0) return 0.0f;

  uint32_t releaseAge = sequencerSampleCounter - audioSequencerGateSamples;
  if (releaseAge >= audioReleaseSamples) return 0.0f;

  return 1.0f - ((float)releaseAge / (float)audioReleaseSamples);
}

float getSequencerStepEdgeFade() {
  if (!sequencerEnabled) return 1.0f;

  int stepSamples = (int)audioSequencerStepSamples;
  int fadeSamples = min(SYNTH_STEP_EDGE_FADE_SAMPLES, max(1, stepSamples / 4));
  uint32_t age = sequencerSampleCounter;
  uint32_t remaining = age < (uint32_t)stepSamples ? (uint32_t)stepSamples - age : 0;
  float fadeIn = clampFloat((float)age / (float)fadeSamples, 0.0f, 1.0f);
  float fadeOut = clampFloat((float)remaining / (float)fadeSamples, 0.0f, 1.0f);

  return min(fadeIn, fadeOut);
}

bool isSequencerStepAudible() {
  return getSequencerGateLevel() > 0.001f;
}

float renderKickSample() {
  if (!kickVoiceActive || audioKickLevel <= 0.01f) return 0.0f;

  if (kickSampleAge >= audioKickMaxSamples) {
    kickVoiceActive = false;
    return 0.0f;
  }

  float t = kickSampleAge / (float)audioKickMaxSamples;
  float tail = 1.0f - t;
  float envelope = tail * tail * tail;
  float pitchDrop = tail * tail;
  float freq = audioKickPitch + audioKickSweep * pitchDrop;
  float click = kickSampleAge < 70 ? (1.0f - kickSampleAge / 70.0f) * (audioKickClick / 100.0f) * 0.5f : 0.0f;

  kickPhase += freq / (float)SYNTH_AUDIO_SAMPLE_RATE;
  if (kickPhase >= 1.0f) kickPhase -= 1.0f;

  kickSampleAge++;

  float sample = getFastSineSample(kickPhase) * envelope + click;
  float drive = 1.0f + audioKickDrive / 35.0f;
  sample *= drive;
  sample = (sample / (1.0f + fabsf(sample))) * 1.7f;
  return sample * (audioKickLevel / 100.0f) * 0.72f;
}

bool isSynthAudioActive() {
  if (!dacI2sStarted) return false;
  if (audioOutputTransition) return false;
  if (uiMode == MODE_BOOT || uiMode == MODE_ABOUT) return false;
  if (isBluetoothStreaming()) return false;

  if (ControlSurface::isLegacyWaveformAudioActive()) {
    return true;
  }

  if (isAudioTestActive()) {
    return true;
  }

  if (isVoicePreviewActive()) {
    return true;
  }

  if (!transportRunning) return false;
  if (!sequencerEnabled && !isMixerLayerAudible()) return false;

  // Silent buffers still drive the sample-accurate transport clock.
  return true;
}

void fillDirectAudioTestBuffer() {
  const float phaseIncrement = (float)AUDIO_TEST_FREQUENCY_HZ / (float)SYNTH_AUDIO_SAMPLE_RATE;

  for (int frame = 0; frame < SYNTH_AUDIO_BUFFER_FRAMES; frame++) {
    int16_t sample = (int16_t)(getFastSineSample(directAudioTestPhase) * AUDIO_TEST_AMPLITUDE);
    directAudioTestPhase += phaseIncrement;
    if (directAudioTestPhase >= 1.0f) directAudioTestPhase -= 1.0f;
    synthAudioBuffer[frame * 2] = sample;
    synthAudioBuffer[frame * 2 + 1] = sample;
    tickSequencerClock();
  }
}

void fillLegacyWaveformAudioBuffer() {
  for (int i = 0; i < SYNTH_AUDIO_BUFFER_FRAMES; i++) {
    float sample = ControlSurface::renderLegacyWaveformSample(synthPhase) * 0.28f;
    synthPhase += 220.0f / (float)SYNTH_AUDIO_SAMPLE_RATE;
    if (synthPhase >= 1.0f) synthPhase -= 1.0f;
    int16_t pcm = (int16_t)(clampFloat(sample, -1.0f, 1.0f) * 32767.0f);

    synthAudioBuffer[i * 2] = pcm;
    synthAudioBuffer[i * 2 + 1] = pcm;
  }
}

void fillSynthAudioBuffer() {
  for (int i = 0; i < SYNTH_AUDIO_BUFFER_FRAMES; i++) {
    tickSequencerClock();

    float mixedSample = 0.0f;
    float gateLevel = getSequencerGateLevel();
    float targetEnvelope = audioVolume > 0.01f ? getSynthEnvelopeLevel() * gateLevel : 0.0f;
    float envelope = getDeclickedEnvelopeLevel(targetEnvelope) * getSequencerStepEdgeFade();

    if (audioVolume > 0.01f && envelope > 0.0005f) {
      float level = (audioVolume / 100.0f) * SYNTH_OUTPUT_GAIN;
      float waveSample = getAudioWaveMixSample() * envelope * level;
      mixedSample = applySynthFilter(waveSample);
    }

    mixedSample += renderKickSample();
    mixedSample = processDelayAndReverb(mixedSample);

    if (!isfinite(mixedSample)) {
      resetSynthDspState(true);
      synthAudioDspRecoveries = synthAudioDspRecoveries + 1;
      mixedSample = 0.0f;
    }

    int16_t pcm = (int16_t)(clampFloat(mixedSample, -1.0f, 1.0f) * 32767.0f);

    synthAudioBuffer[i * 2] = pcm;
    synthAudioBuffer[i * 2 + 1] = pcm;
  }
}

void synthAudioTask(void* parameter) {
  (void)parameter;

  while (true) {
    bool active = isSynthAudioActive();
    bool directTestActive = active && isAudioTestActive();
    bool legacyWaveformActive = active && ControlSurface::isLegacyWaveformAudioActive();

    if (active && (!synthWasActive || synthRetriggerRequested)) {
      if (!synthWasActive && sequencerEnabled) {
        sequencerSampleCounter = 0;
      }

      if (!synthWasActive && kickLevel > 0 && isKickStepEnabled(sequencerCurrentStep)) {
        triggerKickVoice();
      }

      resetSynthVoice();
    }

    if (active) {
      uint32_t renderStartedUs = micros();
      updateAudioParametersForBlock();

      if (directTestActive) {
        if (!directAudioTestWasActive) {
          directAudioTestPhase = 0.0f;
        }
        fillDirectAudioTestBuffer();
      } else if (legacyWaveformActive) {
        fillLegacyWaveformAudioBuffer();
      } else {
        fillSynthAudioBuffer();
      }
      directAudioTestWasActive = directTestActive;

      uint32_t renderElapsedUs = micros() - renderStartedUs;
      uint32_t i2sStartedUs = micros();
      size_t written = btI2s.write((const uint8_t*)synthAudioBuffer, sizeof(synthAudioBuffer));
      uint32_t i2sElapsedUs = micros() - i2sStartedUs;
      if (renderElapsedUs > synthAudioMaxRenderUs) synthAudioMaxRenderUs = renderElapsedUs;
      if (i2sElapsedUs > synthAudioMaxI2sUs) synthAudioMaxI2sUs = i2sElapsedUs;
      synthAudioRenderedBuffers = synthAudioRenderedBuffers + 1;

      if (written != sizeof(synthAudioBuffer)) {
        uint32_t now = millis();
        if (now - synthAudioLastI2sErrorMs >= 1000) {
          synthAudioLastI2sErrorMs = now;
          Serial.print("[AUDIO][I2S] short write=");
          Serial.println(written);
        }
        vTaskDelay(1);
      }
      synthWasActive = true;
    } else {
      if (synthWasActive && !audioOutputTransition && !btAudioStarted) {
        memset(synthAudioBuffer, 0, sizeof(synthAudioBuffer));
        btI2s.write((const uint8_t*)synthAudioBuffer, sizeof(synthAudioBuffer));
        resetSynthDspState(true);
      }

      synthWasActive = false;
      directAudioTestWasActive = false;
      synthEnvelopeSample = 0;
      synthEnvelopeSmoothed = 0.0f;
      vTaskDelay(pdMS_TO_TICKS(8));
    }
  }
}

void startSynthAudio() {
  if (synthAudioTaskHandle != nullptr) return;

  initializeSynthMath();
  resetSynthDspState(true);

  if (xTaskCreatePinnedToCore(synthAudioTask, "lcdSynthAudio", 4096, nullptr, SYNTH_AUDIO_TASK_PRIORITY, &synthAudioTaskHandle, 0) != pdPASS) {
    synthAudioTaskHandle = nullptr;
    Serial.println("[AUDIO] native render task allocation failed");
  } else {
    Serial.println("[AUDIO] native ESP32 render task started");
  }
}
