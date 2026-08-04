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
static uint32_t audioEnvelopeAttackSamples[ENVELOPE_COUNT] = {};
static uint32_t audioEnvelopeDecaySamples[ENVELOPE_COUNT] = {};
static uint32_t audioEnvelopeReleaseSamples[ENVELOPE_COUNT] = {};
static float audioEnvelopeSustainLevels[ENVELOPE_COUNT] = {};
static bool audioModulationRouteEnabled[MOD_SOURCE_COUNT][MOD_DESTINATION_COUNT] = {};
static float audioModulationRouteAmounts[MOD_SOURCE_COUNT][MOD_DESTINATION_COUNT] = {};
static float audioModulationValues[MOD_DESTINATION_COUNT] = {};
static float audioModulatedPitchRatios[WAVE_VOICE_COUNT] = { 1.0f, 1.0f };
static float audioModulatedPulseWidth = 0.5f;
static float audioModulatedFilterResonance = 0.0f;
static float audioModulatedDrive = 0.0f;
static float audioModulatedPan = 0.0f;
// These target values are already part of the destination layer. Future LFO
// source producers can consume them without changing routes, menus or presets.
static float audioModulatedLfoRates[2] = { 0.5f, 1.0f };
static float audioModulatedLfoDepths[2] = { 0.0f, 0.0f };
static uint8_t audioModulationControlCounter = 0;
static uint32_t audioKickMaxSamples = (420U * SYNTH_AUDIO_SAMPLE_RATE) / 1000U;
static uint32_t audioSequencerStepSamples = (SYNTH_AUDIO_SAMPLE_RATE * 15U) / 120U;
static uint32_t audioSequencerGateSamples = ((SYNTH_AUDIO_SAMPLE_RATE * 15U) / 120U) * 70U / 100U;
static bool audioDelayRequested = false;
static bool audioReverbRequested = false;
static bool audioDelayPathActive = false;
static bool audioReverbPathActive = false;

enum AudioEnvelopeStage : uint8_t {
  AUDIO_ENV_IDLE,
  AUDIO_ENV_ATTACK,
  AUDIO_ENV_DECAY,
  AUDIO_ENV_SUSTAIN,
  AUDIO_ENV_RELEASE
};

struct AudioEnvelopeRuntime {
  AudioEnvelopeStage stage;
  float level;
  float releaseStep;
};

static AudioEnvelopeRuntime audioEnvelopeRuntimes[ENVELOPE_COUNT] = {};

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
  audioModulationControlCounter = 0;

  for (int envelope = 0; envelope < ENVELOPE_COUNT; envelope++) {
    audioEnvelopeRuntimes[envelope].stage = AUDIO_ENV_IDLE;
    audioEnvelopeRuntimes[envelope].level = 0.0f;
    audioEnvelopeRuntimes[envelope].releaseStep = 0.0f;
  }

  if (clearEffectBuffers) {
    clearSynthDelayState();
    clearSynthReverbState();
    audioDelayPathActive = false;
    audioReverbPathActive = false;
  }
}

float getCrossfadedVoiceSample(int voiceIndex, float phase, int targetShape, float foldAmount, float pulseWidth) {
  targetShape = clampInt(targetShape, 0, WAVE_SHAPE_COUNT - 1);

  if (targetShape != synthRenderedVoiceShape[voiceIndex]) {
    synthPreviousVoiceShape[voiceIndex] = synthRenderedVoiceShape[voiceIndex];
    synthRenderedVoiceShape[voiceIndex] = targetShape;
    synthVoiceShapeCrossfadeRemaining[voiceIndex] = SYNTH_SHAPE_CROSSFADE_SAMPLES;
  }

  float currentSample = getOscillatorSampleForModParams(
    phase,
    synthRenderedVoiceShape[voiceIndex],
    foldAmount,
    0.0f,
    pulseWidth
  );

  int remaining = synthVoiceShapeCrossfadeRemaining[voiceIndex];
  if (remaining <= 0) return currentSample;

  float previousSample = getOscillatorSampleForModParams(
    phase,
    synthPreviousVoiceShape[voiceIndex],
    foldAmount,
    0.0f,
    pulseWidth
  );
  float mix = 1.0f - ((float)remaining / (float)SYNTH_SHAPE_CROSSFADE_SAMPLES);
  synthVoiceShapeCrossfadeRemaining[voiceIndex] = remaining - 1;

  return previousSample + (currentSample - previousSample) * mix;
}

void updateAudioVoicePhaseSteps() {
  int step = clampInt(sequencerCurrentStep, 0, SEQUENCER_STEP_COUNT - 1);

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    float voiceFrequency = audioFrequency;
    if (sequencerEnabled) {
      float midiPitch = getSequencerStepPitch(i, step) +
                        audioWaveVoiceTune[i] + audioWaveVoiceDetune[i] / 100.0f;
      voiceFrequency = midiPitchToFrequency(clampFloat(
        midiPitch,
        (float)SEQUENCER_MIN_PITCH,
        (float)SEQUENCER_MAX_PITCH
      ));
    } else {
      float semitoneOffset = audioWaveVoiceTune[i] + audioWaveVoiceDetune[i] / 100.0f;
      voiceFrequency *= powf(2.0f, semitoneOffset / 12.0f);
    }

    audioVoicePhaseSteps[i] = voiceFrequency / (float)SYNTH_AUDIO_SAMPLE_RATE;
  }
}

bool isAudioModDestinationRouted(ModDestinationId destination) {
  for (int source = 0; source < MOD_SOURCE_COUNT; source++) {
    if (audioModulationRouteEnabled[source][destination]) return true;
  }
  return false;
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
  for (int envelope = 0; envelope < ENVELOPE_COUNT; envelope++) {
    const AdsrParameters& parameters = envelopeParameters[envelope];
    audioEnvelopeAttackSamples[envelope] = ((uint32_t)clampInt(parameters.attackMs, 0, 500) * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
    audioEnvelopeDecaySamples[envelope] = ((uint32_t)clampInt(parameters.decayMs, 0, 2000) * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
    audioEnvelopeReleaseSamples[envelope] = ((uint32_t)clampInt(parameters.releaseMs, 0, 3000) * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
    audioEnvelopeSustainLevels[envelope] = clampInt(parameters.sustainPercent, 0, 100) / 100.0f;
  }
  for (int source = 0; source < MOD_SOURCE_COUNT; source++) {
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; destination++) {
      audioModulationRouteEnabled[source][destination] = modulationRoutes[source][destination].enabled;
      audioModulationRouteAmounts[source][destination] =
        clampModulationAmount((int)modulationRoutes[source][destination].amountPercent) / 100.0f;
    }
  }

  audioDelayRequested = targetDelayMix > 0 || audioDelayMixAmount > 0.001f ||
                        isAudioModDestinationRouted(MOD_DEST_FX_MIX) ||
                        isAudioModDestinationRouted(MOD_DEST_DELAY_FEEDBACK);
  audioReverbRequested = targetReverbMix > 0 || audioReverbMixAmount > 0.001f ||
                         isAudioModDestinationRouted(MOD_DEST_FX_MIX) ||
                         isAudioModDestinationRouted(MOD_DEST_REVERB_MIX);
  audioKickMaxSamples = (uint32_t)((audioKickDecay * (float)SYNTH_AUDIO_SAMPLE_RATE) / 1000.0f);
  if (audioKickMaxSamples < 80) audioKickMaxSamples = 80;

  int safeBpm = clampInt(sequencerBpm, 40, 240);
  audioSequencerStepSamples = (uint32_t)max(1, (SYNTH_AUDIO_SAMPLE_RATE * 15) / safeBpm);
  audioSequencerGateSamples = (audioSequencerStepSamples * 70U) / 100U;
  if (audioSequencerGateSamples < 1) audioSequencerGateSamples = 1;

  updateAudioVoicePhaseSteps();
}

void updateAudioModulationControlValues(const float sourceValues[]) {
  for (int destination = 0; destination < MOD_DESTINATION_COUNT; destination++) {
    float value = 0.0f;
    for (int source = 0; source < MOD_SOURCE_COUNT; source++) {
      if (!audioModulationRouteEnabled[source][destination]) continue;
      value += sourceValues[source] * audioModulationRouteAmounts[source][destination];
    }
    audioModulationValues[destination] = clampFloat(value, -1.0f, 1.0f);
  }

  audioModulatedPitchRatios[0] = powf(2.0f, audioModulationValues[MOD_DEST_OSC1_PITCH] * 2.0f);
  audioModulatedPitchRatios[1] = powf(2.0f, audioModulationValues[MOD_DEST_OSC2_PITCH] * 2.0f);
  audioModulatedPulseWidth = clampFloat(0.5f + audioModulationValues[MOD_DEST_PWM] * 0.45f, 0.05f, 0.95f);
  audioModulatedFilterResonance = clampFloat(audioModulationValues[MOD_DEST_FILTER_RESONANCE] * 0.95f, 0.0f, 0.95f);
  audioModulatedDrive = clampFloat(audioModulationValues[MOD_DEST_DRIVE], 0.0f, 1.0f);
  audioModulatedPan = clampFloat(audioModulationValues[MOD_DEST_PAN], -1.0f, 1.0f);
  audioModulatedLfoRates[0] = clampFloat(0.5f * powf(2.0f, audioModulationValues[MOD_DEST_LFO1_RATE] * 5.0f), 0.01f, 20.0f);
  audioModulatedLfoRates[1] = clampFloat(1.0f * powf(2.0f, audioModulationValues[MOD_DEST_LFO2_RATE] * 5.0f), 0.01f, 20.0f);
  audioModulatedLfoDepths[0] = clampFloat(audioModulationValues[MOD_DEST_LFO1_DEPTH], -1.0f, 1.0f);
  audioModulatedLfoDepths[1] = clampFloat(audioModulationValues[MOD_DEST_LFO2_DEPTH], -1.0f, 1.0f);

  float cutoff = audioFilterCutoff * powf(2.0f, audioModulationValues[MOD_DEST_FILTER_CUTOFF] * 4.0f);
  cutoff = clampFloat(cutoff, (float)FILTER_MIN_CUTOFF_HZ, (float)FILTER_MAX_CUTOFF_HZ);
  float cutoffRadians = TWO_PI_F * cutoff / (float)SYNTH_AUDIO_SAMPLE_RATE;
  audioFilterCoefficient = clampFloat(1.0f - expf(-cutoffRadians), 0.001f, 0.95f);
}

float getModulatedAmpMultiplier(const float sourceValues[], bool gateOpen) {
  bool hasRoute = false;
  float multiplier = 1.0f;

  for (int source = 0; source < MOD_SOURCE_COUNT; source++) {
    if (!audioModulationRouteEnabled[source][MOD_DEST_AMP_LEVEL]) continue;
    hasRoute = true;
    float amount = audioModulationRouteAmounts[source][MOD_DEST_AMP_LEVEL];
    float shaped = amount >= 0.0f ? sourceValues[source] : 1.0f - sourceValues[source];
    multiplier *= 1.0f - fabsf(amount) + shaped * fabsf(amount);
  }

  return clampFloat(hasRoute ? multiplier : (gateOpen ? 1.0f : 0.0f), 0.0f, 1.0f);
}

float getAudioWaveMixSample() {
  float mixed = 0.0f;
  float totalGain = 0.0f;
  int step = clampInt(sequencerCurrentStep, 0, SEQUENCER_STEP_COUNT - 1);
  int previewVoice = isVoicePreviewActive() ? getCurrentWaveVoiceIndex() : -1;

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    if (!sequencerEnabled && previewVoice >= 0 && i != previewVoice) continue;

    float gain = audioWaveVoiceLevels[i] / 100.0f;
    float mixModulation = audioModulationValues[MOD_DEST_OSC_MIX];
    if (i == 0 && mixModulation > 0.0f) gain *= 1.0f - mixModulation;
    if (i == 1 && mixModulation < 0.0f) gain *= 1.0f + mixModulation;
    gain = clampFloat(gain, 0.0f, 1.0f);
    if (gain <= 0.001f) continue;

    if (sequencerEnabled) {
      if (!isSequencerStepEnabled(i, step)) continue;
    }

    float phase = synthVoicePhases[i];
    if (i == 0 && fabsf(audioModulationValues[MOD_DEST_FM_AMOUNT]) > 0.0001f) {
      phase += getFastSineSample(synthVoicePhases[1]) * audioModulationValues[MOD_DEST_FM_AMOUNT] * 0.20f;
    }
    float foldAmount = clampFloat(
      audioWaveVoiceFold[i] + audioModulationValues[MOD_DEST_WAVEFOLD_AMOUNT] * 100.0f,
      0.0f,
      100.0f
    );
    mixed += getCrossfadedVoiceSample(i, phase, audioWaveVoiceShape[i], foldAmount, audioModulatedPulseWidth) * gain;
    totalGain += gain;

    synthVoicePhases[i] += audioVoicePhaseSteps[i] * audioModulatedPitchRatios[i];
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

  // Resonance is added from the bounded band state, retaining the numerical
  // safety of the one-pole core while exposing a useful Q-like destination.
  float resonanceAccent = filterBandState * audioModulatedFilterResonance * 2.4f;
  float filtered = filterLowState + resonanceAccent;
  if (audioFilterType == 1) {
    filtered = high + resonanceAccent;
  } else if (audioFilterType == 2) {
    filtered = filterBandState * (1.0f + audioModulatedFilterResonance * 2.0f);
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
  float feedback = clampFloat(
    audioDelayFeedbackAmount + audioModulationValues[MOD_DEST_DELAY_FEEDBACK] * 0.90f,
    0.0f,
    0.95f
  );
  float wetMix = clampFloat(
    audioDelayMixAmount + audioModulationValues[MOD_DEST_FX_MIX],
    0.0f,
    1.0f
  );
  float delayStored = input + delayed * feedback;
  delayBuffer[delayWriteIndex] = (int16_t)(clampFloat(delayStored, -1.0f, 1.0f) * 32767.0f);
  delayWriteIndex++;
  if (delayWriteIndex >= DELAY_BUFFER_SAMPLES) delayWriteIndex = 0;

  return input + (delayed - input) * wetMix;
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
  float wetMix = clampFloat(
    audioReverbMixAmount + audioModulationValues[MOD_DEST_REVERB_MIX] + audioModulationValues[MOD_DEST_FX_MIX],
    0.0f,
    1.0f
  );
  return input + (reverbWet - input) * wetMix;
}

float processDelayAndReverb(float input) {
  if (!isfinite(input)) {
    resetSynthDspState(true);
    synthAudioDspRecoveries = synthAudioDspRecoveries + 1;
    return 0.0f;
  }

  return processSynthReverb(processSynthDelay(input));
}

void triggerAudioEnvelopes() {
  for (int envelope = 0; envelope < ENVELOPE_COUNT; envelope++) {
    audioEnvelopeRuntimes[envelope].stage = AUDIO_ENV_ATTACK;
    audioEnvelopeRuntimes[envelope].level = 0.0f;
    audioEnvelopeRuntimes[envelope].releaseStep = 0.0f;
  }
}

float renderAudioEnvelope(int envelope, bool gateOpen) {
  AudioEnvelopeRuntime& runtime = audioEnvelopeRuntimes[envelope];
  float sustainLevel = audioEnvelopeSustainLevels[envelope];

  if (!gateOpen && runtime.stage != AUDIO_ENV_IDLE && runtime.stage != AUDIO_ENV_RELEASE) {
    runtime.stage = AUDIO_ENV_RELEASE;
    uint32_t releaseSamples = audioEnvelopeReleaseSamples[envelope];
    runtime.releaseStep = releaseSamples > 0 ? runtime.level / (float)releaseSamples : runtime.level;
  }

  switch (runtime.stage) {
    case AUDIO_ENV_ATTACK:
      if (!gateOpen) break;
      if (audioEnvelopeAttackSamples[envelope] == 0) {
        runtime.level = 1.0f;
        runtime.stage = AUDIO_ENV_DECAY;
      } else {
        runtime.level += 1.0f / (float)audioEnvelopeAttackSamples[envelope];
        if (runtime.level >= 1.0f) {
          runtime.level = 1.0f;
          runtime.stage = AUDIO_ENV_DECAY;
        }
      }
      break;

    case AUDIO_ENV_DECAY:
      if (!gateOpen) break;
      if (audioEnvelopeDecaySamples[envelope] == 0) {
        runtime.level = sustainLevel;
        runtime.stage = AUDIO_ENV_SUSTAIN;
      } else {
        runtime.level -= (1.0f - sustainLevel) / (float)audioEnvelopeDecaySamples[envelope];
        if (runtime.level <= sustainLevel) {
          runtime.level = sustainLevel;
          runtime.stage = AUDIO_ENV_SUSTAIN;
        }
      }
      break;

    case AUDIO_ENV_SUSTAIN:
      runtime.level = sustainLevel;
      break;

    case AUDIO_ENV_RELEASE:
      if (audioEnvelopeReleaseSamples[envelope] == 0 || runtime.level <= runtime.releaseStep) {
        runtime.level = 0.0f;
        runtime.stage = AUDIO_ENV_IDLE;
      } else {
        runtime.level -= runtime.releaseStep;
      }
      break;

    default:
      runtime.level = 0.0f;
      break;
  }

  return clampFloat(runtime.level, 0.0f, 1.0f);
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
  triggerAudioEnvelopes();
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
  kickTriggerSequence++;
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
      checksum = checksum * 31 + sequencerStepPitches[voice][step];
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
  return 0.0f;
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
  if (uiMode == MODE_BOOT || uiMode == MODE_ABOUT) return false;

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
  }
}

void fillSynthAudioBuffer() {
  for (int i = 0; i < SYNTH_AUDIO_BUFFER_FRAMES; i++) {
    tickSequencerClock();

    float mixedSample = 0.0f;
    bool gateOpen = getSequencerGateLevel() > 0.5f;
    float sourceValues[MOD_SOURCE_COUNT] = {};
    for (int envelope = 0; envelope < ENVELOPE_COUNT; envelope++) {
      sourceValues[getEnvelopeModSource(envelope)] = renderAudioEnvelope(envelope, gateOpen);
    }

    if (audioModulationControlCounter == 0) {
      updateAudioModulationControlValues(sourceValues);
      // Control-rate destinations do not need to run at audio rate. Updating
      // every 32 samples still gives ~1.38 kHz control bandwidth at 44.1 kHz.
      audioModulationControlCounter = 31;
    } else {
      audioModulationControlCounter--;
    }

    float targetAmp = audioVolume > 0.01f ? getModulatedAmpMultiplier(sourceValues, gateOpen) : 0.0f;
    float envelope = getDeclickedEnvelopeLevel(targetAmp) * getSequencerStepEdgeFade();

    if (audioVolume > 0.01f && envelope > 0.0005f) {
      float level = (audioVolume / 100.0f) * SYNTH_OUTPUT_GAIN;
      float waveSample = getAudioWaveMixSample() * envelope * level;
      mixedSample = applySynthFilter(waveSample);

      if (audioModulatedDrive > 0.001f) {
        float driven = mixedSample * (1.0f + audioModulatedDrive * 9.0f);
        mixedSample = driven / (1.0f + fabsf(driven));
      }
    }

    mixedSample += renderKickSample();
    mixedSample = processDelayAndReverb(mixedSample);

    if (!isfinite(mixedSample)) {
      resetSynthDspState(true);
      synthAudioDspRecoveries = synthAudioDspRecoveries + 1;
      mixedSample = 0.0f;
    }

    float leftGain = audioModulatedPan > 0.0f ? 1.0f - audioModulatedPan : 1.0f;
    float rightGain = audioModulatedPan < 0.0f ? 1.0f + audioModulatedPan : 1.0f;
    int16_t leftPcm = (int16_t)(clampFloat(mixedSample * leftGain, -1.0f, 1.0f) * 32767.0f);
    int16_t rightPcm = (int16_t)(clampFloat(mixedSample * rightGain, -1.0f, 1.0f) * 32767.0f);

    synthAudioBuffer[i * 2] = leftPcm;
    synthAudioBuffer[i * 2 + 1] = rightPcm;
  }
}

void synthAudioTask(void* parameter) {
  (void)parameter;

  synthAudioCore = xPortGetCoreID();
  initializeSynthMath();
  resetSynthDspState(true);
  startDacI2s();

  while (true) {
    bool active = isSynthAudioActive();
    bool directTestActive = active && isAudioTestActive();

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

      if (directTestActive) {
        if (!directAudioTestWasActive) {
          directAudioTestPhase = 0.0f;
        }
        fillDirectAudioTestBuffer();
      } else {
        updateAudioParametersForBlock();
        fillSynthAudioBuffer();
      }
      directAudioTestWasActive = directTestActive;

      uint32_t renderElapsedUs = micros() - renderStartedUs;
      uint32_t i2sStartedUs = micros();
      size_t written = audioI2s.write((const uint8_t*)synthAudioBuffer, sizeof(synthAudioBuffer));
      uint32_t i2sElapsedUs = micros() - i2sStartedUs;
      if (renderElapsedUs > synthAudioMaxRenderUs) synthAudioMaxRenderUs = renderElapsedUs;
      if (i2sElapsedUs > synthAudioMaxI2sUs) synthAudioMaxI2sUs = i2sElapsedUs;
      synthAudioRenderedBuffers = synthAudioRenderedBuffers + 1;

      if (written != sizeof(synthAudioBuffer)) {
        synthAudioShortWrites = synthAudioShortWrites + 1;
        synthAudioLastWriteError = audioI2s.lastError();
      }
      if (isSerialDiagnosticsEnabled() && written != sizeof(synthAudioBuffer)) {
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
      if (synthWasActive) {
        memset(synthAudioBuffer, 0, sizeof(synthAudioBuffer));
        size_t written = audioI2s.write((const uint8_t*)synthAudioBuffer, sizeof(synthAudioBuffer));
        if (written != sizeof(synthAudioBuffer)) {
          synthAudioShortWrites = synthAudioShortWrites + 1;
          synthAudioLastWriteError = audioI2s.lastError();
        }
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

  if (xTaskCreatePinnedToCore(synthAudioTask, "lcdSynthAudio", 4096, nullptr, SYNTH_AUDIO_TASK_PRIORITY, &synthAudioTaskHandle, 0) != pdPASS) {
    synthAudioTaskHandle = nullptr;
    Serial.println("[ERROR][AUDIO] native render task allocation failed");
  } else if (isSerialDiagnosticsEnabled()) {
    Serial.println("[AUDIO] native ESP32 render task started");
  }
}
