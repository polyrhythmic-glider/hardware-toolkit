#pragma once

#include "ControlSurfaceBridge.h"

namespace ControlSurfaceBridge {

uint8_t pageBase(uint8_t page) { return (page % PAGE_COUNT) / 3 * 3; }
uint8_t pageSubpage(uint8_t page) { return (page % PAGE_COUNT) % 3; }
static_assert(PAGE_COUNT == 45, "Every page must have three subpages");

const char* subpageName(uint8_t page) {
  static const char* const names[][3] = {
    { "BASE", "PITCH", "SHAPE" }, { "BASE", "PITCH", "SHAPE" },
    { "BASE", "PITCH", "SHAPE" },
    { "BASE", "PITCH", "SHAPE" }, { "BASE", "PITCH", "SHAPE" },
    { "BASE", "PITCH", "SHAPE" },
    { "ADSR", "DYNAMICS", "MODULATION" },
    { "ADSR", "DYNAMICS", "MODULATION" },
    { "ADSR", "DYNAMICS", "MODULATION" },
    { "MOTION", "SHAPE", "BEHAVIOUR" },
    { "MOTION", "SHAPE", "BEHAVIOUR" },
    { "MASTER", "LIVELLI", "STEREO" },
    { "CUTOFF-RES-TYPE", "NOISE-PAN", "MODULATION" },
    { "DELAY", "REVERB", "ROUTING" },
    { "BODY", "TRANSIENT", "DRIVE" }
  };
  return names[pageBase(page) / 3][pageSubpage(page)];
}

bool isOscillatorPage(uint8_t page) { return page < PAGE_ENV_1; }
int getVoiceForPage(uint8_t page) { return isOscillatorPage(page) ? page / 9 : 0; }
int getOscillatorForPage(uint8_t page) { return (page / 3) % 3; }
uint8_t getOscillatorSubpage(uint8_t page) { return pageSubpage(page); }

int getEnvelopeForPage(uint8_t page) {
  if (pageBase(page) == PAGE_ENV_2) return ENVELOPE_ENV2;
  if (pageBase(page) == PAGE_ENV_3) return ENVELOPE_ENV3;
  return ENVELOPE_ENV1;
}

int getLfoForPage(uint8_t page) {
  return pageBase(page) == PAGE_LFO_2 ? 1 : 0;
}

const char* pageMenuName(uint8_t page) {
  static const char* const names[] = {
    "V1 OSC 1", "V1 OSC 2", "V1 OSC 3",
    "V2 OSC 1", "V2 OSC 2", "V2 OSC 3",
    "ENV 1", "ENV 2", "ENV 3", "LFO 1", "LFO 2",
    "PERF", "FILTER", "FX", "909 BD"
  };
  return names[pageBase(page) / 3];
}

const char* pageName(uint8_t page) { return pageMenuName(page); }

// Bind labels, formatting and edits to the same underlying synth parameter.
enum ValueStyle { NUMBER, PERCENT, MILLISECONDS, HERTZ, CENTIHERTZ, TOGGLE,
                  DESTINATION, SOURCE, LFO_WAVE, FILTER_TYPE };
struct ParameterBinding {
  const char* label;
  void* address;
  int (*read)(void*);
  void (*write)(void*, int);
  int minimum, maximum, step;
  ValueStyle style;
};
template <typename T>
ParameterBinding bindParameter(const char* label, T& value, int minimum, int maximum,
                               int step = 1, ValueStyle style = NUMBER) {
  return { label, &value,
    [](void* p) -> int { return *static_cast<T*>(p); },
    [](void* p, int v) { *static_cast<T*>(p) = static_cast<T>(v); },
    minimum, maximum, step, style };
}

uint8_t envelopeDestination[ENVELOPE_COUNT] = {};
bool envelopeSustainSelected[ENVELOPE_COUNT] = {};
uint8_t filterModSource = MOD_SOURCE_ENV1;
uint8_t filterModDestination = MOD_DEST_FILTER_CUTOFF;
uint8_t fxModSource = MOD_SOURCE_ENV1;
uint8_t fxModDestination = MOD_DEST_DELAY_MIX;

ParameterBinding parameterBinding(uint8_t page, uint8_t encoderIndex) {
  int envelope = getEnvelopeForPage(page);
  int lfo = getLfoForPage(page);
  int slot = pageSubpage(page) * 3 + encoderIndex % 3;
  switch (pageBase(page)) {
    case PAGE_ENV_1:
    case PAGE_ENV_2:
    case PAGE_ENV_3:
      switch (slot) {
        case 0: return bindParameter("ATTACK", envelopeParameters[envelope].attackMs, 0, 500, 1, MILLISECONDS);
        case 1: return envelopeSustainSelected[envelope] ? bindParameter("SUSTAIN *E2", envelopeParameters[envelope].sustainPercent, 0, 100, 1, PERCENT) : bindParameter("DECAY *E2", envelopeParameters[envelope].decayMs, 0, 2000, 5, MILLISECONDS);
        case 2: return bindParameter("RELEASE", envelopeParameters[envelope].releaseMs, 0, 3000, 20, MILLISECONDS);
        case 3: return bindParameter("AMOUNT", envelopeAmountPercent[envelope], -100, 100, 1, PERCENT);
        case 4: return bindParameter("VELOCITY", envelopeVelocitySensitivityPercent[envelope], 0, 100, 1, PERCENT);
        case 5: return bindParameter("RETRIGGER", envelopeRetriggerMode[envelope], 0, 2, 1, NUMBER);
        case 6: return bindParameter("DEST", envelopeDestination[envelope], 0, MOD_DESTINATION_COUNT - 1, 1, DESTINATION);
        case 7: return bindParameter("ROUTE ON", modulationRoutes[getEnvelopeModSource(envelope)][envelopeDestination[envelope]].enabled, 0, 1, 1, TOGGLE);
        case 8: return bindParameter("ROUTE AMT", modulationRoutes[getEnvelopeModSource(envelope)][envelopeDestination[envelope]].amountPercent, -100, 100, 1, PERCENT);
      }
      break;
    case PAGE_LFO_1:
    case PAGE_LFO_2:
      switch (slot) {
        case 0: return bindParameter("RATE", lfoParameters[lfo].frequencyCentiHz, 0, 20000, 5, CENTIHERTZ);
        case 1: return bindParameter("DEPTH", lfoParameters[lfo].amountPercent, -100, 100, 1, PERCENT);
        case 2: return bindParameter("DEST", lfoParameters[lfo].performanceDestination, 0, MOD_DESTINATION_COUNT - 1, 1, DESTINATION);
        case 3: return bindParameter("WAVE", lfoParameters[lfo].waveform, 0, LFO_WAVE_COUNT - 1, 1, LFO_WAVE);
        case 4: return bindParameter("PHASE", lfoParameters[lfo].phasePercent, 0, 100, 1, PERCENT);
        case 5: return bindParameter("BIPOLAR", lfoParameters[lfo].bipolar, 0, 1, 1, TOGGLE);
        case 6: return bindParameter("RETRIGGER", lfoParameters[lfo].retriggerOnNoteOn, 0, 1, 1, TOGGLE);
        case 7: return bindParameter("FREE RUN", lfoParameters[lfo].freeRunning, 0, 1, 1, TOGGLE);
        case 8: return bindParameter("TEMPO SYNC", lfoParameters[lfo].tempoSync, 0, 1, 1, TOGGLE);
      }
      break;
    case PAGE_MIXER:
      switch (slot) {
        case 0: return bindParameter("MASTER", volume, 0, 100, 1, PERCENT);
        case 1: return bindParameter("TUNE", frequency, 20, 2000, 1, HERTZ);
        case 2: return bindParameter("TEMPO", sequencerBpm, 40, 240, 1, NUMBER);
        case 3: return bindParameter("V1 LEVEL", voiceOscillators[0].masterLevelPercent, 0, 100, 1, PERCENT);
        case 4: return bindParameter("V2 LEVEL", voiceOscillators[1].masterLevelPercent, 0, 100, 1, PERCENT);
        case 5: return bindParameter("909 LEVEL", kickLevel, 0, 100, 1, PERCENT);
        case 6: return bindParameter("V1 O1 PAN", oscillatorPanPercent[0], -100, 100, 1, PERCENT);
        case 7: return bindParameter("V1 O2 PAN", oscillatorPanPercent[1], -100, 100, 1, PERCENT);
        case 8: return bindParameter("V1 O3 PAN", oscillatorPanPercent[2], -100, 100, 1, PERCENT);
      }
      break;
    case PAGE_FILTER_FX:
      switch (slot) {
        case 0: return bindParameter("CUTOFF", filterCutoff, FILTER_MIN_CUTOFF_HZ, FILTER_MAX_CUTOFF_HZ, 50, HERTZ);
        case 1: return bindParameter("RESONANCE", filterResonance, 0, 100, 1, PERCENT);
        case 2: return bindParameter("TYPE", filterType, 0, FILTER_TYPE_COUNT - 1, 1, FILTER_TYPE);
        case 3: return bindParameter("NOISE", synthNoiseLevel, 0, 100, 1, PERCENT);
        case 4: return bindParameter("PAN", synthPan, -100, 100, 1, PERCENT);
        case 5: return bindParameter("FILTER ON", filterEnabled, 0, 1, 1, TOGGLE);
        case 6: return bindParameter("SOURCE", filterModSource, 0, MOD_SOURCE_COUNT - 1, 1, SOURCE);
        case 7: return bindParameter("DEST", filterModDestination, MOD_DEST_FILTER_CUTOFF, MOD_DEST_FILTER_RESONANCE, 1, DESTINATION);
        case 8: return bindParameter("AMOUNT", modulationRoutes[filterModSource][filterModDestination].amountPercent, -100, 100, 1, PERCENT);
      }
      break;
    case PAGE_DELAY_REVERB:
      switch (slot) {
        case 0: return bindParameter("DELAY MIX", delayMix, 0, 100, 1, PERCENT);
        case 1: return bindParameter("TIME", delayTimeMs, 20, DELAY_MAX_MS, 10, MILLISECONDS);
        case 2: return bindParameter("FEEDBACK", delayFeedback, 0, 90, 1, PERCENT);
        case 3: return bindParameter("REVERB MIX", reverbMix, 0, 100, 1, PERCENT);
        case 4: return bindParameter("ROOM", reverbRoom, 0, 95, 1, PERCENT);
        case 5: return bindParameter("DELAY SEND", delayMix, 0, 100, 1, PERCENT);
        case 6: return bindParameter("SOURCE", fxModSource, 0, MOD_SOURCE_COUNT - 1, 1, SOURCE);
        case 7: return bindParameter("DEST", fxModDestination, MOD_DEST_DELAY_MIX, MOD_DEST_REVERB_MIX, 1, DESTINATION);
        case 8: return bindParameter("AMOUNT", modulationRoutes[fxModSource][fxModDestination].amountPercent, -100, 100, 1, PERCENT);
      }
      break;
    case PAGE_KICK:
      switch (slot) {
        case 0: return bindParameter("LEVEL", kickLevel, 0, 100, 1, PERCENT);
        case 1: return bindParameter("TUNE", kickPitch, 25, 120, 1, HERTZ);
        case 2: return bindParameter("DECAY", kickDecay, 80, 1200, 10, MILLISECONDS);
        case 3: return bindParameter("ATTACK", kickAttack, 0, 100, 1, PERCENT);
        case 4: return bindParameter("SWEEP", kickSweep, 0, 260, 5, HERTZ);
        case 5: return bindParameter("TUNE", kickPitch, 25, 120, 1, HERTZ);
        case 6: return bindParameter("DRIVE", kickDrive, 0, 100, 1, PERCENT);
        case 7: return bindParameter("LEVEL", kickLevel, 0, 100, 1, PERCENT);
        case 8: return bindParameter("ATTACK", kickAttack, 0, 100, 1, PERCENT);
      }
      break;

    default: break;
  }
  return bindParameter("VALUE", volume, 0, 100, 1, PERCENT);
}

const char* parameterLabel(uint8_t page, uint8_t encoderIndex) {
  OscillatorBank& bank = voiceOscillators[getVoiceForPage(page)];
  encoderIndex %= 3;
  static const char* const labels[3][3] = {
    { "WAVE", "LEVEL", "FOLD" }, { "OCTAVE", "TUNE", "DETUNE" },
    { "PHASE", "PULSE", "PAN" }
  };
  if (!isOscillatorPage(page)) return parameterBinding(page, encoderIndex).label;
  if (getOscillatorSubpage(page) == 0 && encoderIndex == 0)
    return bank.oscillatorEnabled[getOscillatorForPage(page)] ? "WAVE ON" : "WAVE OFF";
  return labels[getOscillatorSubpage(page)][encoderIndex];
}

void formatParameterValue(uint8_t page,
                          uint8_t encoderIndex,
                          char* output,
                          size_t outputSize) {
  OscillatorBank& bank = voiceOscillators[getVoiceForPage(page)];
  if (output == nullptr || outputSize == 0) return;
  page %= PAGE_COUNT;
  encoderIndex %= 3;

  if (isOscillatorPage(page)) {
    int oscillator = getOscillatorForPage(page);
    uint8_t subpage = getOscillatorSubpage(page);
    if (subpage == 0 && encoderIndex == 0) {
      snprintf(output, outputSize, "%s", waveformNames[bank.waveVoiceShape[oscillator]]);
    } else if (subpage == 0 && encoderIndex == 1) {
      snprintf(output, outputSize, "%d%%", bank.waveVoiceLevels[oscillator]);
    } else if (subpage == 0) {
      snprintf(output, outputSize, "%d%%", bank.waveVoiceFold[oscillator]);
    } else if (subpage == 1 && encoderIndex == 0) {
      snprintf(output, outputSize, "%+d oct", bank.oscillatorOctave[oscillator]);
    } else if (subpage == 1 && encoderIndex == 1) {
      snprintf(output, outputSize, "%+d st", bank.waveVoiceTune[oscillator]);
    } else if (subpage == 1) {
      snprintf(output, outputSize, "%+d ct", bank.waveVoiceDetune[oscillator]);
    } else if (encoderIndex == 0) {
      snprintf(output, outputSize, "%d%%", bank.oscillatorPhasePercent[oscillator]);
    } else if (encoderIndex == 1) {
      snprintf(output, outputSize, "%d%%", bank.oscillatorPulseWidthPercent[oscillator]);
    } else {
      snprintf(output, outputSize, "%+d%%", bank.oscillatorPanPercent[oscillator]);
    }
    return;
  }

  ParameterBinding parameter = parameterBinding(page, encoderIndex);
  int value = parameter.read(parameter.address);
  switch (parameter.style) {
    case TOGGLE: snprintf(output, outputSize, "%s", value ? "ON" : "OFF"); break;
    case DESTINATION: snprintf(output, outputSize, "%s", MOD_DESTINATIONS[value].tag); break;
    case SOURCE: snprintf(output, outputSize, "%s %d", value < 3 ? "ENV" : "LFO", value < 3 ? value + 1 : value - 2); break;
    case LFO_WAVE: snprintf(output, outputSize, "%s", LFO_WAVEFORM_NAMES[value]); break;
    case FILTER_TYPE: snprintf(output, outputSize, "%s", FILTER_TYPE_NAMES[value]); break;
    case PERCENT: snprintf(output, outputSize, "%d%%", value); break;
    case MILLISECONDS: snprintf(output, outputSize, "%d ms", value); break;
    case HERTZ: snprintf(output, outputSize, "%d Hz", value); break;
    case CENTIHERTZ: snprintf(output, outputSize, "%d.%02d Hz", value / 100, value % 100); break;
    default: snprintf(output, outputSize, "%d", value); break;
  }
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
  OscillatorBank& bank = voiceOscillators[getVoiceForPage(page)];
  if (delta == 0) return;
  page %= PAGE_COUNT;
  encoderIndex %= 3;

  if (isOscillatorPage(page)) {
    int oscillator = getOscillatorForPage(page);
    uint8_t subpage = getOscillatorSubpage(page);
    if (subpage == 0 && encoderIndex == 0) {
      bank.waveVoiceShape[oscillator] = wrapIndex(
        bank.waveVoiceShape[oscillator] + delta, WAVE_SHAPE_COUNT
      );
    } else if (subpage == 0 && encoderIndex == 1) {
      bank.waveVoiceLevels[oscillator] = clampInt(
        bank.waveVoiceLevels[oscillator] + delta * 2, 0, 100
      );
    } else if (subpage == 0) {
      bank.waveVoiceFold[oscillator] = clampInt(
        bank.waveVoiceFold[oscillator] + delta * 2, 0, 100
      );
    } else if (subpage == 1 && encoderIndex == 0) {
      bank.oscillatorOctave[oscillator] = clampInt(bank.oscillatorOctave[oscillator] + delta, -4, 4);
    } else if (subpage == 1 && encoderIndex == 1) {
      bank.waveVoiceTune[oscillator] = clampInt(bank.waveVoiceTune[oscillator] + delta, -48, 48);
    } else if (subpage == 1) {
      bank.waveVoiceDetune[oscillator] = clampInt(bank.waveVoiceDetune[oscillator] + delta, -100, 100);
    } else if (encoderIndex == 0) {
      bank.oscillatorPhasePercent[oscillator] = wrapIndex(bank.oscillatorPhasePercent[oscillator] + delta, 101);
    } else if (encoderIndex == 1) {
      bank.oscillatorPulseWidthPercent[oscillator] = clampInt(
        bank.oscillatorPulseWidthPercent[oscillator] + delta, 1, 99
      );
    } else {
      bank.oscillatorPanPercent[oscillator] = clampInt(
        bank.oscillatorPanPercent[oscillator] + delta * 2, -100, 100
      );
    }
  } else {
    ParameterBinding parameter = parameterBinding(page, encoderIndex);
    int value = clampInt(parameter.read(parameter.address) + delta * parameter.step,
                         parameter.minimum, parameter.maximum);
    if ((pageBase(page) == PAGE_LFO_1 || pageBase(page) == PAGE_LFO_2) &&
        pageSubpage(page) == 0 && encoderIndex == 2) {
      selectLfoPerformanceDestination(getLfoForPage(page), value);
    } else {
      parameter.write(parameter.address, value);
    }
    if (page == PAGE_FILTER_FX_MODULATION && encoderIndex == 2)
      modulationRoutes[filterModSource][filterModDestination].enabled = true;
    if (page == PAGE_DELAY_REVERB_ROUTING && encoderIndex == 2)
      modulationRoutes[fxModSource][fxModDestination].enabled = true;
  }

  valueDirty = true;
  fullRedraw = false;
}

void activateParameterAction(uint8_t page, uint8_t encoderIndex) {
  if (pageSubpage(page) == 0 && pageBase(page) >= PAGE_ENV_1 &&
      pageBase(page) <= PAGE_ENV_3 && encoderIndex == 1) {
    int envelope = getEnvelopeForPage(page);
    envelopeSustainSelected[envelope] = !envelopeSustainSelected[envelope];
  }
}

void activatePageAction(uint8_t page) {
  OscillatorBank& bank = voiceOscillators[getVoiceForPage(page)];
  page = pageBase(page);
  if (isOscillatorPage(page)) {
    int oscillator = getOscillatorForPage(page);
    bank.oscillatorEnabled[oscillator] = !bank.oscillatorEnabled[oscillator];
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
  auto addValue = [&hash](int value) {
    hash ^= (uint32_t)value;
    hash *= 16777619u;
  };
  for (const OscillatorBank& bank : voiceOscillators) {
    addValue(bank.masterLevelPercent);
    for (int oscillator = 0; oscillator < SYNTH_OSCILLATOR_COUNT; ++oscillator) {
      const int values[] = {
        bank.oscillatorEnabled[oscillator], bank.waveVoiceShape[oscillator],
        bank.waveVoiceLevels[oscillator], bank.waveVoiceTune[oscillator],
        bank.waveVoiceFold[oscillator], bank.waveVoiceDetune[oscillator],
        bank.oscillatorOctave[oscillator], bank.oscillatorPhasePercent[oscillator],
        bank.oscillatorPulseWidthPercent[oscillator], bank.oscillatorPanPercent[oscillator]
      };
      for (int value : values) addValue(value);
    }
  }
  for (uint8_t page = PAGE_ENV_1; page < PAGE_COUNT; ++page) {
    for (uint8_t encoder = 0; encoder < 3; ++encoder) {
      ParameterBinding parameter = parameterBinding(page, encoder);
      addValue(parameter.read(parameter.address));
    }
  }
  for (int envelope = 0; envelope < ENVELOPE_COUNT; ++envelope) {
    addValue(envelopeEnabled[envelope]);
    addValue(envelopeParameters[envelope].decayMs);
    addValue(envelopeParameters[envelope].sustainPercent);
    addValue(envelopeSustainSelected[envelope]);
  }
  for (int lfo = 0; lfo < SYNTH_LFO_COUNT; ++lfo) addValue(lfoParameters[lfo].enabled);
  for (int source = 0; source < MOD_SOURCE_COUNT; ++source) {
    for (int destination = 0; destination < MOD_DESTINATION_COUNT; ++destination) {
      addValue(modulationRoutes[source][destination].enabled);
      addValue(modulationRoutes[source][destination].amountPercent);
    }
  }
  return hash;
}

bool isTransportRunning() {
  return transportRunning;
}

}  // namespace ControlSurfaceBridge
