#pragma once

#include "ControlSurfaceBridge.h"

namespace ControlSurfaceBridge {

const char* pageName(uint8_t page) {
  switch (page % PAGE_COUNT) {
    case PAGE_VOICE_1: return "V1 SHAPER";
    case PAGE_VOICE_2: return "V2 SHAPER";
    case PAGE_TUNING: return "TUNING";
    case PAGE_ENVELOPE: return "ENVELOPE";
    case PAGE_ENVELOPE_CLOCK: return "ENV + CLOCK";
    case PAGE_MIXER: return "MIXER";
    case PAGE_FILTER_FX: return "FILTER";
    case PAGE_DELAY_REVERB: return "DELAY + REV";
    case PAGE_KICK: return "KICK";
    case PAGE_KICK_SHAPE: return "KICK SHAPE";
    case PAGE_SEQUENCER: return "SEQUENCER";
    case PAGE_TRANSPORT: return "TRANSPORT";
    default: return "SYNTH";
  }
}

const char* parameterLabel(uint8_t page, uint8_t encoderIndex) {
  encoderIndex %= 3;

  static const char* VOICE_LABELS[] = { "WAVE", "FOLD", "PHASE" };
  static const char* TUNING_LABELS[] = { "PITCH", "V1 TUNE", "V2 TUNE" };
  static const char* ENVELOPE_LABELS[] = { "ATTACK", "DECAY", "SUSTAIN" };
  static const char* ENVELOPE_CLOCK_LABELS[] = { "RELEASE", "GATE", "BPM" };
  static const char* MIXER_LABELS[] = { "MASTER", "VOICE 1", "VOICE 2" };
  static const char* FILTER_LABELS[] = { "CUTOFF", "TYPE", "POWER" };
  static const char* DELAY_REVERB_LABELS[] = { "DELAY", "FEEDBACK", "REVERB" };
  static const char* KICK_LABELS[] = { "LEVEL", "PITCH", "DECAY" };
  static const char* KICK_SHAPE_LABELS[] = { "SWEEP", "CLICK", "DRIVE" };
  static const char* SEQUENCER_LABELS[] = { "VOICE", "STEP", "NOTE" };
  static const char* TRANSPORT_LABELS[] = { "LENGTH", "DIRECTION", "TRANSPOSE" };

  switch (page % PAGE_COUNT) {
    case PAGE_VOICE_1:
    case PAGE_VOICE_2:
      return VOICE_LABELS[encoderIndex];
    case PAGE_TUNING:
      return TUNING_LABELS[encoderIndex];
    case PAGE_ENVELOPE:
      return ENVELOPE_LABELS[encoderIndex];
    case PAGE_ENVELOPE_CLOCK:
      return ENVELOPE_CLOCK_LABELS[encoderIndex];
    case PAGE_MIXER:
      return MIXER_LABELS[encoderIndex];
    case PAGE_FILTER_FX:
      return FILTER_LABELS[encoderIndex];
    case PAGE_DELAY_REVERB:
      return DELAY_REVERB_LABELS[encoderIndex];
    case PAGE_KICK:
      return KICK_LABELS[encoderIndex];
    case PAGE_KICK_SHAPE:
      return KICK_SHAPE_LABELS[encoderIndex];
    case PAGE_SEQUENCER:
      return SEQUENCER_LABELS[encoderIndex];
    case PAGE_TRANSPORT:
      return TRANSPORT_LABELS[encoderIndex];
    default:
      return "VALUE";
  }
}

void formatParameterValue(uint8_t page, uint8_t encoderIndex, char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;

  page %= PAGE_COUNT;
  encoderIndex %= 3;

  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2) {
    int voice = page == PAGE_VOICE_1 ? 0 : 1;
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%s", waveformNames[waveVoiceShape[voice]]); return;
      case 1: snprintf(output, outputSize, "%d%%", waveVoiceFold[voice]); return;
      default: snprintf(output, outputSize, "%d deg", waveVoicePhase[voice]); return;
    }
  }

  if (page == PAGE_TUNING) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d Hz", frequency); return;
      case 1: snprintf(output, outputSize, "%+d st", waveVoiceTune[0]); return;
      default: snprintf(output, outputSize, "%+d st", waveVoiceTune[1]); return;
    }
  }

  if (page == PAGE_ENVELOPE) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d ms", attack); return;
      case 1: snprintf(output, outputSize, "%d ms", decay); return;
      case 2: snprintf(output, outputSize, "%d%%", sustain); return;
      default: snprintf(output, outputSize, "%d%%", sustain); return;
    }
  }

  if (page == PAGE_ENVELOPE_CLOCK) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d ms", releaseMs); return;
      case 1: snprintf(output, outputSize, "%d%%", sequencerGate); return;
      default: snprintf(output, outputSize, "%d", sequencerBpm); return;
    }
  }

  if (page == PAGE_MIXER) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d%%", volume); return;
      case 1: snprintf(output, outputSize, "%d%%", waveVoiceLevels[0]); return;
      case 2: snprintf(output, outputSize, "%d%%", waveVoiceLevels[1]); return;
      default: snprintf(output, outputSize, "%d%%", kickLevel); return;
    }
  }

  if (page == PAGE_FILTER_FX) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d Hz", filterCutoff); return;
      case 1: snprintf(output, outputSize, "%s", FILTER_TYPE_NAMES[filterType]); return;
      default: snprintf(output, outputSize, "%s", filterEnabled ? "ON" : "OFF"); return;
    }
  }

  if (page == PAGE_DELAY_REVERB) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d%%", delayMix); return;
      case 1: snprintf(output, outputSize, "%d%%", delayFeedback); return;
      default: snprintf(output, outputSize, "%d%%", reverbMix); return;
    }
  }

  if (page == PAGE_KICK) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d%%", kickLevel); return;
      case 1: snprintf(output, outputSize, "%d Hz", kickPitch); return;
      default: snprintf(output, outputSize, "%d ms", kickDecay); return;
    }
  }

  if (page == PAGE_KICK_SHAPE) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d Hz", kickSweep); return;
      case 1: snprintf(output, outputSize, "%d%%", kickClick); return;
      default: snprintf(output, outputSize, "%d%%", kickDrive); return;
    }
  }

  if (page == PAGE_SEQUENCER) {
    int voice = getActiveSequencerVoice();
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "V%d", voice + 1); return;
      case 1: snprintf(output, outputSize, "%d/%d", sequencerSelectedStep + 1, getSequencerLength()); return;
      default: snprintf(output, outputSize, "%+d st", getSelectedSequencerStepNote()); return;
    }
  }

  switch (encoderIndex) {
    case 0: snprintf(output, outputSize, "%d", getSequencerLength()); return;
    case 1: snprintf(output, outputSize, "%s", SEQUENCER_DIRECTION_NAMES[sequencerDirection]); return;
    default: snprintf(output, outputSize, "%+d st", sequencerTranspose); return;
  }
}

void adjustParameter(uint8_t page, uint8_t encoderIndex, int delta) {
  if (delta == 0) return;

  page %= PAGE_COUNT;
  encoderIndex %= 3;

  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2) {
    int voice = page == PAGE_VOICE_1 ? 0 : 1;
    switch (encoderIndex) {
      case 0:
        setWaveVoiceShapeForTrack(voice, waveVoiceShape[voice] + delta);
        break;
      case 1:
        waveVoiceFold[voice] = clampInt(waveVoiceFold[voice] + delta * 2, 0, 100);
        break;
      default:
        waveVoicePhase[voice] = wrapIndex(waveVoicePhase[voice] + delta * 5, 360);
        break;
    }
  } else if (page == PAGE_TUNING) {
    switch (encoderIndex) {
      case 0: frequency = clampInt(frequency + delta * 5, 20, 2000); break;
      case 1: waveVoiceTune[0] = clampInt(waveVoiceTune[0] + delta, -24, 24); break;
      default: waveVoiceTune[1] = clampInt(waveVoiceTune[1] + delta, -24, 24); break;
    }
  } else if (page == PAGE_ENVELOPE) {
    switch (encoderIndex) {
      case 0: attack = clampInt(attack + delta, 0, 500); break;
      case 1: decay = clampInt(decay + delta * 5, 0, 2000); break;
      default: sustain = clampInt(sustain + delta, 0, 100); break;
    }
  } else if (page == PAGE_ENVELOPE_CLOCK) {
    switch (encoderIndex) {
      case 0: releaseMs = clampInt(releaseMs + delta * 20, 0, 3000); break;
      case 1: sequencerGate = clampInt(sequencerGate + delta * 5, 5, 100); break;
      default: sequencerBpm = clampInt(sequencerBpm + delta * 2, 40, 240); break;
    }
  } else if (page == PAGE_MIXER) {
    switch (encoderIndex) {
      case 0: volume = clampInt(volume + delta, 0, 100); break;
      case 1: waveVoiceLevels[0] = clampInt(waveVoiceLevels[0] + delta * 5, 0, 100); break;
      default: waveVoiceLevels[1] = clampInt(waveVoiceLevels[1] + delta * 5, 0, 100); break;
    }
  } else if (page == PAGE_FILTER_FX) {
    switch (encoderIndex) {
      case 0: filterCutoff = clampInt(filterCutoff + delta * 50, FILTER_MIN_CUTOFF_HZ, FILTER_MAX_CUTOFF_HZ); break;
      case 1: filterType = wrapIndex(filterType + delta, FILTER_TYPE_COUNT); break;
      default: filterEnabled = !filterEnabled; break;
    }
  } else if (page == PAGE_DELAY_REVERB) {
    switch (encoderIndex) {
      case 0: delayMix = clampInt(delayMix + delta * 5, 0, 100); break;
      case 1: delayFeedback = clampInt(delayFeedback + delta * 5, 0, 90); break;
      default: reverbMix = clampInt(reverbMix + delta * 5, 0, 100); break;
    }
  } else if (page == PAGE_KICK) {
    switch (encoderIndex) {
      case 0:
        kickLevel = clampInt(kickLevel + delta * 5, 0, 100);
        if (kickLevel > 0) triggerKickVoice();
        break;
      case 1: kickPitch = clampInt(kickPitch + delta, 25, 120); break;
      default: kickDecay = clampInt(kickDecay + delta * 10, 80, 1200); break;
    }
  } else if (page == PAGE_KICK_SHAPE) {
    switch (encoderIndex) {
      case 0: kickSweep = clampInt(kickSweep + delta * 5, 0, 260); break;
      case 1: kickClick = clampInt(kickClick + delta * 5, 0, 100); break;
      default: kickDrive = clampInt(kickDrive + delta * 5, 0, 100); break;
    }
  } else if (page == PAGE_SEQUENCER) {
    switch (encoderIndex) {
      case 0:
        activeSequencerVoice = wrapIndex(activeSequencerVoice + delta, WAVE_VOICE_COUNT);
        synthRetriggerRequested = true;
        break;
      case 1:
        sequencerSelectedStep = wrapIndex(sequencerSelectedStep + delta, getSequencerLength());
        if (!sequencerEnabled && !isMixerLayerEnabled()) {
          sequencerCurrentStep = sequencerSelectedStep;
          resetSequencerClock();
        }
        break;
      default:
      {
        int voice = getActiveSequencerVoice();
        sequencerStepNotes[voice][sequencerSelectedStep] = clampInt(
          sequencerStepNotes[voice][sequencerSelectedStep] + delta,
          SEQUENCER_MIN_NOTE,
          SEQUENCER_MAX_NOTE
        );
        sequencerStepEnabled[voice][sequencerSelectedStep] = true;
        synthRetriggerRequested = true;
        break;
      }
    }
  } else {
    switch (encoderIndex) {
      case 0:
        sequencerLength = clampInt(sequencerLength + delta, 1, SEQUENCER_STEP_COUNT);
        clampSequencerIndexes();
        resetSequencerClock();
        break;
      case 1:
        sequencerDirection = wrapIndex(sequencerDirection + delta, SEQUENCER_DIRECTION_COUNT);
        sequencerPingPongForward = true;
        resetSequencerClock();
        break;
      default: sequencerTranspose = clampInt(sequencerTranspose + delta, SEQUENCER_MIN_TRANSPOSE, SEQUENCER_MAX_TRANSPOSE); break;
    }
  }

  valueDirty = true;
  fullRedraw = false;
}

void activatePageAction(uint8_t page) {
  page %= PAGE_COUNT;

  if (page == PAGE_MIXER) {
    triggerKickVoice();
  } else if (page == PAGE_FILTER_FX) {
    filterEnabled = !filterEnabled;
  } else if (page == PAGE_KICK || page == PAGE_KICK_SHAPE) {
    triggerKickVoice();
  } else if (page == PAGE_SEQUENCER) {
    int voice = getActiveSequencerVoice();
    sequencerStepEnabled[voice][sequencerSelectedStep] = !sequencerStepEnabled[voice][sequencerSelectedStep];
  } else if (page == PAGE_TRANSPORT) {
    if (transportRunning) stopTransport(); else startTransport();
  } else {
    synthRetriggerRequested = true;
  }

  valueDirty = true;
  fullRedraw = false;
}

const char* pageActionLabel(uint8_t page) {
  switch (page % PAGE_COUNT) {
    case PAGE_MIXER: return "E3: KICK";
    case PAGE_FILTER_FX: return filterEnabled ? "E3: FILTER OFF" : "E3: FILTER ON";
    case PAGE_KICK:
    case PAGE_KICK_SHAPE: return "E3: TRIGGER KICK";
    case PAGE_SEQUENCER: return isSelectedSequencerStepEnabled() ? "E3: STEP OFF" : "E3: STEP ON";
    case PAGE_TRANSPORT: return transportRunning ? "E3: STOP" : "E3: START";
    default: return "E3: RETRIGGER";
  }
}

uint32_t stateHash() {
  uint32_t hash = 2166136261u;

  const int values[] = {
    waveVoiceShape[0], waveVoiceShape[1], waveVoiceTune[0], waveVoiceTune[1],
    waveVoiceFold[0], waveVoiceFold[1], waveVoicePhase[0], waveVoicePhase[1],
    waveVoiceLevels[0], waveVoiceLevels[1], frequency,
    attack, decay, sustain, releaseMs, volume, kickLevel,
    kickPitch, kickDecay, kickSweep, kickClick, kickDrive,
    filterCutoff, filterType, filterEnabled ? 1 : 0,
    delayMix, delayFeedback, reverbMix,
    sequencerBpm, activeSequencerVoice, sequencerSelectedStep, sequencerLength,
    getSelectedSequencerStepNote(), isSelectedSequencerStepEnabled() ? 1 : 0,
    sequencerDirection, sequencerGate, sequencerTranspose, transportRunning ? 1 : 0
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    hash ^= (uint32_t)values[i];
    hash *= 16777619u;
  }

  return hash;
}

bool isTransportRunning() {
  return transportRunning;
}

}  // namespace ControlSurfaceBridge
