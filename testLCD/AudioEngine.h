#pragma once

float applyWaveFold(float sample) {
  return applyWaveFoldAmount(sample, (float)waveFold);
}

float getOscillatorSample(float phase) {
  return getOscillatorSampleForParams(phase, waveform, (float)waveFold, (float)wavePhase);
}

void updateAudioParameterSmoothing() {
  audioFrequency = smoothToward(audioFrequency, (float)frequency, 0.0012f);
  audioVolume = smoothToward(audioVolume, (float)volume, 0.0010f);
  audioWaveFold = smoothToward(audioWaveFold, (float)waveFold, 0.0012f);
  audioWavePhase = smoothAngleDegrees(audioWavePhase, (float)wavePhase, 0.0010f);
  audioKickLevel = smoothToward(audioKickLevel, (float)kickLevel, 0.0015f);
  audioKickPitch = smoothToward(audioKickPitch, (float)kickPitch, 0.0015f);
  audioKickDecay = smoothToward(audioKickDecay, (float)kickDecay, 0.0015f);
  audioKickSweep = smoothToward(audioKickSweep, (float)kickSweep, 0.0015f);
  audioKickClick = smoothToward(audioKickClick, (float)kickClick, 0.0015f);
  audioKickDrive = smoothToward(audioKickDrive, (float)kickDrive, 0.0015f);
  audioFilterAmount = smoothToward(audioFilterAmount, filterEnabled ? 1.0f : 0.0f, 0.0015f);
  audioFilterCutoff = smoothToward(audioFilterCutoff, (float)filterCutoff, 0.0015f);
  audioFilterType = filterType;

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    audioWaveVoiceLevels[i] = smoothToward(audioWaveVoiceLevels[i], (float)waveVoiceLevels[i], 0.0015f);
    audioWaveVoiceShape[i] = waveVoiceShape[i];
    audioWaveVoiceTune[i] = smoothToward(audioWaveVoiceTune[i], (float)waveVoiceTune[i], 0.0012f);
    audioWaveVoiceFold[i] = smoothToward(audioWaveVoiceFold[i], (float)waveVoiceFold[i], 0.0012f);
    audioWaveVoicePhase[i] = smoothAngleDegrees(audioWaveVoicePhase[i], (float)waveVoicePhase[i], 0.0010f);
  }
}

float getAudioWaveMixSample(float baseFrequency) {
  if (uiMode == MODE_EDIT && getSelectedAction() == ACTION_AUDIO_TEST) {
    float sample = getOscillatorSampleForParams(synthPhase, waveform, audioWaveFold, audioWavePhase);
    synthPhase += baseFrequency / (float)SYNTH_AUDIO_SAMPLE_RATE;

    if (synthPhase >= 1.0f) {
      synthPhase -= floorf(synthPhase);
    }

    return sample;
  }

  float mixed = 0.0f;
  float totalGain = 0.0f;

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    float gain = audioWaveVoiceLevels[i] / 100.0f;
    if (gain <= 0.001f) continue;

    float tuneRatio = powf(2.0f, audioWaveVoiceTune[i] / 12.0f);
    float phaseStep = (baseFrequency * tuneRatio) / (float)SYNTH_AUDIO_SAMPLE_RATE;

    mixed += getOscillatorSampleForParams(synthVoicePhases[i], audioWaveVoiceShape[i], audioWaveVoiceFold[i], audioWaveVoicePhase[i]) * gain;
    totalGain += gain;

    synthVoicePhases[i] += phaseStep;
    if (synthVoicePhases[i] >= 1.0f) {
      synthVoicePhases[i] -= floorf(synthVoicePhases[i]);
    }
  }

  if (totalGain <= 0.001f) return 0.0f;

  float normalizer = totalGain < 1.0f ? 1.0f : totalGain;
  return mixed / normalizer;
}

float applySynthFilter(float input) {
  if (audioFilterAmount <= 0.001f) return input;

  float safeCutoff = clampFloat(audioFilterCutoff, (float)FILTER_MIN_CUTOFF_HZ, (float)FILTER_MAX_CUTOFF_HZ);
  float f = 2.0f * sinf((3.1415926f * safeCutoff) / (float)SYNTH_AUDIO_SAMPLE_RATE);
  f = clampFloat(f, 0.001f, 0.95f);

  const float damping = 0.72f;
  filterLowState += f * filterBandState;
  float high = input - filterLowState - damping * filterBandState;
  filterBandState += f * high;

  float filtered = filterLowState;
  if (audioFilterType == 1) {
    filtered = high;
  } else if (audioFilterType == 2) {
    filtered = filterBandState;
  }

  return input + (filtered - input) * audioFilterAmount;
}

float processDelayAndReverb(float input) {
  int delaySamples = clampInt((delayTimeMs * SYNTH_AUDIO_SAMPLE_RATE) / 1000, 1, DELAY_BUFFER_SAMPLES - 1);
  uint32_t delayReadIndex = (delayWriteIndex + DELAY_BUFFER_SAMPLES - delaySamples) % DELAY_BUFFER_SAMPLES;
  float delayed = delayBuffer[delayReadIndex] / 32767.0f;
  float delayStored = input + delayed * (delayFeedback / 100.0f);
  delayBuffer[delayWriteIndex] = (int16_t)(clampFloat(delayStored, -1.0f, 1.0f) * 32767.0f);
  delayWriteIndex = (delayWriteIndex + 1) % DELAY_BUFFER_SAMPLES;

  float delayOutput = input + (delayed - input) * (delayMix / 100.0f);
  float room = reverbRoom / 100.0f;
  float combA = reverbBufferA[reverbIndexA] / 32767.0f;
  float combB = reverbBufferB[reverbIndexB] / 32767.0f;
  float combC = reverbBufferC[reverbIndexC] / 32767.0f;
  reverbBufferA[reverbIndexA] = (int16_t)(clampFloat(delayOutput + combA * room, -1.0f, 1.0f) * 32767.0f);
  reverbBufferB[reverbIndexB] = (int16_t)(clampFloat(delayOutput + combB * room * 0.96f, -1.0f, 1.0f) * 32767.0f);
  reverbBufferC[reverbIndexC] = (int16_t)(clampFloat(delayOutput + combC * room * 0.92f, -1.0f, 1.0f) * 32767.0f);
  reverbIndexA = (reverbIndexA + 1) % REVERB_BUFFER_A_SAMPLES;
  reverbIndexB = (reverbIndexB + 1) % REVERB_BUFFER_B_SAMPLES;
  reverbIndexC = (reverbIndexC + 1) % REVERB_BUFFER_C_SAMPLES;

  float reverbWet = (combA + combB + combC) / 3.0f;
  return delayOutput + (reverbWet - delayOutput) * (reverbMix / 100.0f);
}

float getSynthEnvelopeLevel() {
  float sustainLevel = sustain / 100.0f;
  uint32_t attackSamples = ((uint32_t)attack * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
  uint32_t decaySamples = ((uint32_t)decay * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
  float level = sustainLevel;

  if (attackSamples > 0 && synthEnvelopeSample < attackSamples) {
    level = (float)synthEnvelopeSample / (float)attackSamples;
  } else if (decaySamples > 0 && synthEnvelopeSample < attackSamples + decaySamples) {
    float t = (float)(synthEnvelopeSample - attackSamples) / (float)decaySamples;
    level = 1.0f - (1.0f - sustainLevel) * clampFloat(t, 0.0f, 1.0f);
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

void triggerKickVoice() {
  kickPhase = 0.0f;
  kickSampleAge = 0;
  kickVoiceActive = kickLevel > 0;
}

int getSequencerStepSamples() {
  return max(1, (SYNTH_AUDIO_SAMPLE_RATE * 15) / sequencerBpm);
}

int getSequencerGateSamples() {
  int stepSamples = getSequencerStepSamples();
  return clampInt((stepSamples * sequencerGate) / 100, 1, stepSamples);
}

int getSequencerNoteChecksum() {
  int checksum = 0;

  for (int i = 0; i < SEQUENCER_STEP_COUNT; i++) {
    checksum = checksum * 31 + sequencerStepNotes[i];
    checksum = checksum * 3 + (sequencerStepEnabled[i] ? 1 : 0);
  }

  return checksum;
}

int getWaveVoiceParamChecksum() {
  int checksum = 0;

  for (int i = 0; i < WAVE_VOICE_COUNT; i++) {
    checksum = checksum * 31 + waveVoiceShape[i];
    checksum = checksum * 31 + waveVoiceTune[i];
    checksum = checksum * 31 + waveVoiceFold[i];
    checksum = checksum * 31 + waveVoicePhase[i];
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
  sequencerPingPongForward = true;
}

void tickSequencerClock() {
  if (!isPatternClockActive()) return;

  sequencerSampleCounter++;

  if (sequencerSampleCounter >= (uint32_t)getSequencerStepSamples()) {
    int length = getSequencerLength();

    sequencerSampleCounter = 0;

    switch (sequencerDirection) {
      case 1:
        sequencerCurrentStep = (sequencerCurrentStep + length - 1) % length;
        break;

      case 2:
        if (length <= 1) {
          sequencerCurrentStep = 0;
        } else if (sequencerPingPongForward) {
          if (sequencerCurrentStep >= length - 1) {
            sequencerPingPongForward = false;
            sequencerCurrentStep--;
          } else {
            sequencerCurrentStep++;
          }
        } else {
          if (sequencerCurrentStep <= 0) {
            sequencerPingPongForward = true;
            sequencerCurrentStep++;
          } else {
            sequencerCurrentStep--;
          }
        }
        break;

      case 3:
        sequencerCurrentStep = random(0, length);
        break;

      default:
        sequencerCurrentStep = (sequencerCurrentStep + 1) % length;
        break;
    }

    if (kickLevel > 0 && (sequencerCurrentStep == 0 || (getSequencerLength() > 4 && sequencerCurrentStep == 4))) {
      triggerKickVoice();
    }

    resetSynthVoice();
  }
}

float getSequencerGateLevel() {
  if (!sequencerEnabled) return 1.0f;

  int step = sequencerCurrentStep;
  if (step < 0 || step >= SEQUENCER_STEP_COUNT) return 0.0f;
  if (!sequencerStepEnabled[step]) return 0.0f;

  uint32_t gateSamples = (uint32_t)getSequencerGateSamples();
  if (sequencerSampleCounter < gateSamples) return 1.0f;

  uint32_t releaseSamples = ((uint32_t)releaseMs * SYNTH_AUDIO_SAMPLE_RATE) / 1000;
  if (releaseSamples == 0) return 0.0f;

  uint32_t releaseAge = sequencerSampleCounter - gateSamples;
  if (releaseAge >= releaseSamples) return 0.0f;

  return 1.0f - ((float)releaseAge / (float)releaseSamples);
}

bool isSequencerStepAudible() {
  return getSequencerGateLevel() > 0.001f;
}

float getActiveSynthFrequency() {
  float activeFrequency = audioFrequency;

  if (sequencerEnabled) {
    int step = sequencerCurrentStep;
    if (step < 0 || step >= SEQUENCER_STEP_COUNT) {
      step = 0;
    }

    activeFrequency *= powf(2.0f, (sequencerStepNotes[step] + sequencerTranspose) / 12.0f);
  }

  return clampFloat(activeFrequency, 20.0f, 2000.0f);
}

float renderKickSample() {
  if (!kickVoiceActive || audioKickLevel <= 0.01f) return 0.0f;

  uint32_t maxSamples = (uint32_t)((audioKickDecay * (float)SYNTH_AUDIO_SAMPLE_RATE) / 1000.0f);
  if (maxSamples < 80) maxSamples = 80;

  if (kickSampleAge >= maxSamples) {
    kickVoiceActive = false;
    return 0.0f;
  }

  float t = kickSampleAge / (float)maxSamples;
  float tail = 1.0f - t;
  float envelope = tail * tail * tail;
  float pitchDrop = tail * tail;
  float freq = audioKickPitch + audioKickSweep * pitchDrop;
  float click = kickSampleAge < 70 ? (1.0f - kickSampleAge / 70.0f) * (audioKickClick / 100.0f) * 0.5f : 0.0f;

  kickPhase += freq / (float)SYNTH_AUDIO_SAMPLE_RATE;
  if (kickPhase >= 1.0f) {
    kickPhase -= floorf(kickPhase);
  }

  kickSampleAge++;

  float sample = sinf(kickPhase * TWO_PI_F) * envelope + click;
  float drive = 1.0f + audioKickDrive / 35.0f;
  sample *= drive;
  sample = (sample / (1.0f + fabsf(sample))) * 1.7f;
  return sample * (audioKickLevel / 100.0f) * 0.72f;
}

bool isSynthAudioActive() {
  if (!dacI2sStarted) return false;
  if (uiMode == MODE_BOOT || uiMode == MODE_ABOUT) return false;
  if (isBluetoothStreaming()) return false;
  if (!transportRunning) return false;

  return volume > 0 || audioVolume > 0.05f || isMixerLayerAudible();
}

void fillSynthAudioBuffer(bool active) {
  for (int i = 0; i < SYNTH_AUDIO_BUFFER_FRAMES; i++) {
    int16_t pcm = 0;

    if (active) {
      tickSequencerClock();
      updateAudioParameterSmoothing();

      float mixedSample = 0.0f;

      float gateLevel = getSequencerGateLevel();
      float targetEnvelope = audioVolume > 0.01f ? getSynthEnvelopeLevel() * gateLevel : 0.0f;
      float envelope = getDeclickedEnvelopeLevel(targetEnvelope);

      if (audioVolume > 0.01f && envelope > 0.0005f) {
        float activeFrequency = getActiveSynthFrequency();
        float level = (audioVolume / 100.0f) * SYNTH_OUTPUT_GAIN;
        float waveSample = getAudioWaveMixSample(activeFrequency) * envelope * level;
        mixedSample += applySynthFilter(waveSample);
      }

      mixedSample += renderKickSample();
      mixedSample = processDelayAndReverb(mixedSample);

      pcm = (int16_t)(clampFloat(mixedSample, -1.0f, 1.0f) * 32767.0f);
    }

    synthAudioBuffer[i * 2] = pcm;
    synthAudioBuffer[i * 2 + 1] = pcm;
  }
}

void synthAudioTask(void* parameter) {
  (void)parameter;

  while (true) {
    bool active = isSynthAudioActive();

    if (active && (!synthWasActive || synthRetriggerRequested)) {
      if (!synthWasActive && sequencerEnabled) {
        sequencerSampleCounter = 0;
      }

      if (!synthWasActive && kickLevel > 0) {
        triggerKickVoice();
      }

      resetSynthVoice();
    }

    if (active) {
      fillSynthAudioBuffer(true);
      btI2s.write((const uint8_t*)synthAudioBuffer, sizeof(synthAudioBuffer));
      synthWasActive = true;
    } else {
      if (synthWasActive && !isBluetoothStreaming()) {
        fillSynthAudioBuffer(false);
        btI2s.write((const uint8_t*)synthAudioBuffer, sizeof(synthAudioBuffer));
      }

      synthWasActive = false;
      synthEnvelopeSample = 0;
      synthEnvelopeSmoothed = 0.0f;
      vTaskDelay(pdMS_TO_TICKS(8));
    }
  }
}

void startSynthAudio() {
  if (synthAudioTaskHandle != nullptr) return;

  if (xTaskCreatePinnedToCore(synthAudioTask, "lcdSynthAudio", 4096, nullptr, 1, &synthAudioTaskHandle, 0) != pdPASS) {
    synthAudioTaskHandle = nullptr;
  }
}
