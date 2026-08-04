#pragma once

#include "ControlSurfaceBridge.h"

namespace ControlSurfaceBridge {

int getOscillatorForPage(uint8_t page) {
  if (page == PAGE_VOICE_2) return 1;
  if (page == PAGE_VOICE_3) return 2;
  return 0;
}

int getEnvelopeForPage(uint8_t page) {
  if (page == PAGE_ENV_2) return ENVELOPE_ENV2;
  if (page == PAGE_ENV_3) return ENVELOPE_ENV3;
  return ENVELOPE_ENV1;
}

int getLfoForPage(uint8_t page) {
  return page == PAGE_LFO_2 ? 1 : 0;
}

const char* pageName(uint8_t page) {
  switch (page % PAGE_COUNT) {
    case PAGE_VOICE_1: return "OSCILLATOR 1";
    case PAGE_VOICE_2: return "OSCILLATOR 2";
    case PAGE_VOICE_3: return "OSCILLATOR 3";
    case PAGE_ENV_1: return "ENVELOPE 1";
    case PAGE_ENV_2: return "ENVELOPE 2";
    case PAGE_ENV_3: return "ENVELOPE 3";
    case PAGE_LFO_1: return "LFO 1";
    case PAGE_LFO_2: return "LFO 2";
    case PAGE_MIXER: return "PERFORMANCE";
    case PAGE_FILTER_FX: return "FILTER";
    case PAGE_DELAY_REVERB: return "DELAY + REV";
    case PAGE_KICK: return "909 MAIN";
    case PAGE_KICK_SHAPE: return "909 SHAPE";
    default: return "SYNTH";
  }
}

const char* parameterLabel(uint8_t page, uint8_t encoderIndex) {
  encoderIndex %= 3;
  static const char* OSCILLATOR_LABELS[] = { "WAVE", "LEVEL", "FOLD" };
  static const char* ENVELOPE_LABELS[] = { "ATTACK", "DECAY", "RELEASE" };
  static const char* LFO_LABELS[] = { "RATE", "DEPTH", "DEST" };
  static const char* PERFORMANCE_LABELS[] = { "MASTER", "NOISE", "PAN" };
  static const char* FILTER_LABELS[] = { "CUTOFF", "RESONANCE", "TYPE" };
  static const char* FX_LABELS[] = { "DELAY", "FEEDBACK", "REVERB" };
  static const char* KICK_LABELS[] = { "LEVEL", "TUNE", "ATTACK" };
  static const char* KICK_SHAPE_LABELS[] = { "DECAY", "SWEEP", "DRIVE" };

  if ((page == PAGE_VOICE_1 || page == PAGE_VOICE_2 || page == PAGE_VOICE_3) &&
      encoderIndex == 0) {
    return oscillatorEnabled[getOscillatorForPage(page)] ? "WAVE ON" : "WAVE OFF";
  }

  switch (page % PAGE_COUNT) {
    case PAGE_VOICE_1:
    case PAGE_VOICE_2:
    case PAGE_VOICE_3: return OSCILLATOR_LABELS[encoderIndex];
    case PAGE_ENV_1:
    case PAGE_ENV_2:
    case PAGE_ENV_3: return ENVELOPE_LABELS[encoderIndex];
    case PAGE_LFO_1:
    case PAGE_LFO_2: return LFO_LABELS[encoderIndex];
    case PAGE_MIXER: return PERFORMANCE_LABELS[encoderIndex];
    case PAGE_FILTER_FX: return FILTER_LABELS[encoderIndex];
    case PAGE_DELAY_REVERB: return FX_LABELS[encoderIndex];
    case PAGE_KICK: return KICK_LABELS[encoderIndex];
    case PAGE_KICK_SHAPE: return KICK_SHAPE_LABELS[encoderIndex];
    default: return "VALUE";
  }
}

void formatParameterValue(uint8_t page,
                          uint8_t encoderIndex,
                          char* output,
                          size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  page %= PAGE_COUNT;
  encoderIndex %= 3;

  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2 || page == PAGE_VOICE_3) {
    int oscillator = getOscillatorForPage(page);
    if (encoderIndex == 0) {
      snprintf(output, outputSize, "%s", waveformNames[waveVoiceShape[oscillator]]);
    } else if (encoderIndex == 1) {
      snprintf(output, outputSize, "%d%%", waveVoiceLevels[oscillator]);
    } else {
      snprintf(output, outputSize, "%d%%", waveVoiceFold[oscillator]);
    }
    return;
  }

  if (page == PAGE_ENV_1 || page == PAGE_ENV_2 || page == PAGE_ENV_3) {
    int envelope = getEnvelopeForPage(page);
    if (encoderIndex == 0) {
      snprintf(output, outputSize, "%d ms", envelopeParameters[envelope].attackMs);
    } else if (encoderIndex == 1) {
      snprintf(output, outputSize, "%d ms", envelopeParameters[envelope].decayMs);
    } else {
      snprintf(output,
               outputSize,
               "%d ms %s",
               envelopeParameters[envelope].releaseMs,
               envelopeEnabled[envelope] ? "ON" : "OFF");
    }
    return;
  }

  if (page == PAGE_LFO_1 || page == PAGE_LFO_2) {
    int lfo = getLfoForPage(page);
    if (encoderIndex == 0) {
      snprintf(output,
               outputSize,
               "%d.%02d Hz",
               lfoParameters[lfo].frequencyCentiHz / 100,
               abs(lfoParameters[lfo].frequencyCentiHz % 100));
    } else if (encoderIndex == 1) {
      snprintf(output,
               outputSize,
               "%+d%% %s",
               lfoParameters[lfo].amountPercent,
               lfoParameters[lfo].enabled ? "ON" : "OFF");
    } else {
      snprintf(output,
               outputSize,
               "%s",
               MOD_DESTINATIONS[lfoParameters[lfo].performanceDestination].tag);
    }
    return;
  }

  if (page == PAGE_MIXER) {
    if (encoderIndex == 0) snprintf(output, outputSize, "%d%%", volume);
    else if (encoderIndex == 1) snprintf(output, outputSize, "%d%%", synthNoiseLevel);
    else snprintf(output, outputSize, "%+d%%", synthPan);
    return;
  }

  if (page == PAGE_FILTER_FX) {
    if (encoderIndex == 0) snprintf(output, outputSize, "%d Hz", filterCutoff);
    else if (encoderIndex == 1) snprintf(output, outputSize, "%d%%", filterResonance);
    else snprintf(output,
                  outputSize,
                  "%s %s",
                  FILTER_TYPE_NAMES[filterType],
                  filterEnabled ? "ON" : "OFF");
    return;
  }

  if (page == PAGE_DELAY_REVERB) {
    if (encoderIndex == 0) snprintf(output, outputSize, "%d%%", delayMix);
    else if (encoderIndex == 1) snprintf(output, outputSize, "%d%%", delayFeedback);
    else snprintf(output, outputSize, "%d%%", reverbMix);
    return;
  }

  if (page == PAGE_KICK) {
    if (encoderIndex == 0) snprintf(output, outputSize, "%d%%", kickLevel);
    else if (encoderIndex == 1) snprintf(output, outputSize, "%d Hz", kickPitch);
    else snprintf(output, outputSize, "%d%%", kickAttack);
    return;
  }

  if (encoderIndex == 0) snprintf(output, outputSize, "%d ms", kickDecay);
  else if (encoderIndex == 1) snprintf(output, outputSize, "%d Hz", kickSweep);
  else snprintf(output, outputSize, "%d%%", kickDrive);
}

bool isOscillatorSummaryPage(uint8_t) {
  return false;
}

void formatOscillatorSummaryTitle(char* output, size_t outputSize) {
  if (output != nullptr && outputSize > 0) snprintf(output, outputSize, "OSCILLATOR");
}

void formatOscillatorSummaryLine(uint8_t, char* output, size_t outputSize) {
  if (output != nullptr && outputSize > 0) output[0] = '\0';
}

void selectLfoPerformanceDestination(int lfo, int destination) {
  lfo = clampInt(lfo, 0, SYNTH_LFO_COUNT - 1);
  destination = wrapIndex(destination, MOD_DESTINATION_COUNT);
  int source = lfo == 0 ? MOD_SOURCE_LFO1 : MOD_SOURCE_LFO2;
  for (int index = 0; index < MOD_DESTINATION_COUNT; ++index) {
    modulationRoutes[source][index].enabled = false;
    modulationRoutes[source][index].amountPercent = 0;
  }
  lfoParameters[lfo].performanceDestination = (uint8_t)destination;
  modulationRoutes[source][destination].enabled = true;
  modulationRoutes[source][destination].amountPercent = 100;
}

void adjustParameter(uint8_t page, uint8_t encoderIndex, int delta) {
  if (delta == 0) return;
  page %= PAGE_COUNT;
  encoderIndex %= 3;

  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2 || page == PAGE_VOICE_3) {
    int oscillator = getOscillatorForPage(page);
    if (encoderIndex == 0) {
      waveVoiceShape[oscillator] = wrapIndex(
        waveVoiceShape[oscillator] + delta, WAVE_SHAPE_COUNT
      );
    } else if (encoderIndex == 1) {
      waveVoiceLevels[oscillator] = clampInt(
        waveVoiceLevels[oscillator] + delta * 2, 0, 100
      );
    } else {
      waveVoiceFold[oscillator] = clampInt(
        waveVoiceFold[oscillator] + delta * 2, 0, 100
      );
    }
  } else if (page == PAGE_ENV_1 || page == PAGE_ENV_2 || page == PAGE_ENV_3) {
    int envelope = getEnvelopeForPage(page);
    if (encoderIndex == 0) {
      envelopeParameters[envelope].attackMs = clampInt(
        envelopeParameters[envelope].attackMs + delta, 0, 500
      );
    } else if (encoderIndex == 1) {
      envelopeParameters[envelope].decayMs = clampInt(
        envelopeParameters[envelope].decayMs + delta * 5, 0, 2000
      );
    } else {
      envelopeParameters[envelope].releaseMs = clampInt(
        envelopeParameters[envelope].releaseMs + delta * 20, 0, 3000
      );
    }
  } else if (page == PAGE_LFO_1 || page == PAGE_LFO_2) {
    int lfo = getLfoForPage(page);
    if (encoderIndex == 0) {
      lfoParameters[lfo].frequencyCentiHz = clampInt(
        lfoParameters[lfo].frequencyCentiHz + delta * 5, 0, 20000
      );
    } else if (encoderIndex == 1) {
      lfoParameters[lfo].amountPercent = clampInt(
        lfoParameters[lfo].amountPercent + delta, -100, 100
      );
    } else {
      selectLfoPerformanceDestination(
        lfo, lfoParameters[lfo].performanceDestination + delta
      );
    }
  } else if (page == PAGE_MIXER) {
    if (encoderIndex == 0) volume = clampInt(volume + delta, 0, 100);
    else if (encoderIndex == 1) {
      synthNoiseLevel = clampInt(synthNoiseLevel + delta * 2, 0, 100);
    } else {
      synthPan = clampInt(synthPan + delta * 2, -100, 100);
    }
  } else if (page == PAGE_FILTER_FX) {
    if (encoderIndex == 0) {
      filterCutoff = clampInt(
        filterCutoff + delta * 50, FILTER_MIN_CUTOFF_HZ, FILTER_MAX_CUTOFF_HZ
      );
    } else if (encoderIndex == 1) {
      filterResonance = clampInt(filterResonance + delta * 2, 0, 100);
    } else {
      filterType = wrapIndex(filterType + delta, FILTER_TYPE_COUNT);
    }
  } else if (page == PAGE_DELAY_REVERB) {
    if (encoderIndex == 0) delayMix = clampInt(delayMix + delta * 5, 0, 100);
    else if (encoderIndex == 1) {
      delayFeedback = clampInt(delayFeedback + delta * 5, 0, 90);
    } else {
      reverbMix = clampInt(reverbMix + delta * 5, 0, 100);
    }
  } else if (page == PAGE_KICK) {
    if (encoderIndex == 0) kickLevel = clampInt(kickLevel + delta * 5, 0, 100);
    else if (encoderIndex == 1) kickPitch = clampInt(kickPitch + delta, 25, 120);
    else kickAttack = clampInt(kickAttack + delta * 5, 0, 100);
  } else if (page == PAGE_KICK_SHAPE) {
    if (encoderIndex == 0) kickDecay = clampInt(kickDecay + delta * 10, 80, 1200);
    else if (encoderIndex == 1) kickSweep = clampInt(kickSweep + delta * 5, 0, 260);
    else kickDrive = clampInt(kickDrive + delta * 5, 0, 100);
  }

  valueDirty = true;
  fullRedraw = false;
}

void activatePageAction(uint8_t page) {
  page %= PAGE_COUNT;
  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2 || page == PAGE_VOICE_3) {
    int oscillator = getOscillatorForPage(page);
    oscillatorEnabled[oscillator] = !oscillatorEnabled[oscillator];
  } else if (page == PAGE_ENV_1 || page == PAGE_ENV_2 || page == PAGE_ENV_3) {
    int envelope = getEnvelopeForPage(page);
    envelopeEnabled[envelope] = !envelopeEnabled[envelope];
  } else if (page == PAGE_LFO_1 || page == PAGE_LFO_2) {
    int lfo = getLfoForPage(page);
    lfoParameters[lfo].enabled = !lfoParameters[lfo].enabled;
  } else if (page == PAGE_FILTER_FX) {
    filterEnabled = !filterEnabled;
  } else if (page == PAGE_KICK || page == PAGE_KICK_SHAPE) {
    triggerKickVoice();
  }

  valueDirty = true;
  fullRedraw = false;
}

uint32_t stateHash() {
  uint32_t hash = 2166136261u;
  for (int oscillator = 0; oscillator < SYNTH_OSCILLATOR_COUNT; ++oscillator) {
    const int values[] = {
      oscillatorEnabled[oscillator] ? 1 : 0,
      waveVoiceShape[oscillator],
      waveVoiceLevels[oscillator],
      waveVoiceTune[oscillator],
      waveVoiceFold[oscillator],
      waveVoiceDetune[oscillator]
    };
    for (size_t value = 0; value < sizeof(values) / sizeof(values[0]); ++value) {
      hash ^= (uint32_t)values[value];
      hash *= 16777619u;
    }
  }
  for (int envelope = 0; envelope < ENVELOPE_COUNT; ++envelope) {
    const int values[] = {
      envelopeEnabled[envelope] ? 1 : 0,
      envelopeParameters[envelope].attackMs,
      envelopeParameters[envelope].decayMs,
      envelopeParameters[envelope].sustainPercent,
      envelopeParameters[envelope].releaseMs
    };
    for (size_t value = 0; value < sizeof(values) / sizeof(values[0]); ++value) {
      hash ^= (uint32_t)values[value];
      hash *= 16777619u;
    }
  }
  for (int lfo = 0; lfo < SYNTH_LFO_COUNT; ++lfo) {
    hash ^= (uint32_t)lfoParameters[lfo].enabled;
    hash *= 16777619u;
    hash ^= (uint32_t)lfoParameters[lfo].frequencyCentiHz;
    hash *= 16777619u;
    hash ^= (uint32_t)lfoParameters[lfo].amountPercent;
    hash *= 16777619u;
    hash ^= (uint32_t)lfoParameters[lfo].performanceDestination;
    hash *= 16777619u;
  }
  const int performanceValues[] = {
    volume, synthNoiseLevel, synthPan, filterCutoff, filterResonance,
    filterType, filterEnabled ? 1 : 0, delayMix, delayFeedback, reverbMix,
    kickLevel, kickPitch, kickAttack, kickDecay, kickSweep, kickDrive
  };
  for (size_t value = 0;
       value < sizeof(performanceValues) / sizeof(performanceValues[0]);
       ++value) {
    hash ^= (uint32_t)performanceValues[value];
    hash *= 16777619u;
  }
  return hash;
}

bool isTransportRunning() {
  return transportRunning;
}

}  // namespace ControlSurfaceBridge
