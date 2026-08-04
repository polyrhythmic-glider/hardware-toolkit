#pragma once

#include "ControlSurfaceBridge.h"

namespace ControlSurfaceBridge {

uint8_t activeTrackPage() {
  switch (getActiveSequencerTrack()) {
    case SEQUENCER_TRACK_VOICE_2: return PAGE_VOICE_2;
    case SEQUENCER_TRACK_KICK: return PAGE_KICK;
    default: return PAGE_VOICE_1;
  }
}

const char* pageName(uint8_t page) {
  switch (page % PAGE_COUNT) {
    case PAGE_VOICE_1: return "V1 PARAMETERS";
    case PAGE_VOICE_2: return "V2 PARAMETERS";
    case PAGE_TUNING: return getActiveSequencerTrackVoice() == 1 ? "OSCILLATOR 2" : "OSCILLATOR 1";
    case PAGE_ENVELOPE: return "ENVELOPE";
    case PAGE_ENVELOPE_CLOCK: return "ENV + CLOCK";
    case PAGE_MIXER: return "MIXER";
    case PAGE_FILTER_FX: return "FILTER";
    case PAGE_DELAY_REVERB: return "DELAY + REV";
    case PAGE_KICK: return "909 MAIN";
    case PAGE_KICK_SHAPE: return "909 SHAPE";
    case PAGE_SEQUENCER: return "SEQUENCER";
    case PAGE_TRANSPORT: return "TRANSPORT";
    default: return "SYNTH";
  }
}

const char* parameterLabel(uint8_t page, uint8_t encoderIndex) {
  page %= PAGE_COUNT;

  static const char* VOICE_LABELS[] = { "WAVE", "TUNE", "FOLD", "DETUNE" };
  static const char* KICK_LABELS[] = { "LEVEL", "TUNE", "ATTACK" };
  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2) return VOICE_LABELS[encoderIndex % 4];
  if (page == PAGE_KICK) return KICK_LABELS[encoderIndex % 3];

  encoderIndex %= 3;

  static const char* TUNING_LABELS[] = { "PITCH", "TUNE", "FINE" };
  static const char* ENVELOPE_LABELS[] = { "ATTACK", "DECAY", "SUSTAIN" };
  static const char* ENVELOPE_CLOCK_LABELS[] = { "RELEASE", "BPM", "LENGTH" };
  static const char* MIXER_LABELS[] = { "MASTER", "TRACK", "NOTE" };
  static const char* FILTER_LABELS[] = { "CUTOFF", "TYPE", "POWER" };
  static const char* DELAY_REVERB_LABELS[] = { "DELAY", "FEEDBACK", "REVERB" };
  static const char* KICK_SHAPE_LABELS[] = { "DECAY", "SWEEP", "DRIVE" };
  static const char* SEQUENCER_LABELS[] = { "STEP", "NOTE", "STATE" };
  static const char* TRANSPORT_LABELS[] = { "LENGTH", "BPM", "STEP" };

  switch (page) {
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
    case PAGE_KICK_SHAPE:
      return KICK_SHAPE_LABELS[encoderIndex];
    case PAGE_SEQUENCER:
      if (encoderIndex == 1) {
        static char stepLabel[24];
        snprintf(stepLabel, sizeof(stepLabel), "%s S%02d",
                 isActiveSequencerTrackKick() ? "HIT" : "NOTE",
                 sequencerSelectedStep + 1);
        return stepLabel;
      }
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

  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2) {
    encoderIndex %= 4;
    int voice = page == PAGE_VOICE_1 ? 0 : 1;
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%s", waveformNames[waveVoiceShape[voice]]); return;
      case 1: snprintf(output, outputSize, "%+d st", waveVoiceTune[voice]); return;
      case 2: snprintf(output, outputSize, "%d%%", waveVoiceFold[voice]); return;
      default: snprintf(output, outputSize, "%+d ct", waveVoiceDetune[voice]); return;
    }
  }

  if (page == PAGE_KICK) {
    switch (encoderIndex % 3) {
      case 0: snprintf(output, outputSize, "%d%%", kickLevel); return;
      case 1: snprintf(output, outputSize, "%d Hz", kickPitch); return;
      default: snprintf(output, outputSize, "%d%%", kickAttack); return;
    }
  }

  encoderIndex %= 3;

  if (page == PAGE_TUNING) {
    int voice = getActiveSequencerTrackVoice();
    switch (encoderIndex) {
      case 0: formatMidiPitch(getSelectedSequencerStepPitch(), output, outputSize); return;
      case 1: snprintf(output, outputSize, "%+d st", waveVoiceTune[voice]); return;
      default: snprintf(output, outputSize, "%+d ct", waveVoiceDetune[voice]); return;
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
      case 1: snprintf(output, outputSize, "%d", sequencerBpm); return;
      default: snprintf(output, outputSize, "%d", getSequencerLength()); return;
    }
  }

  if (page == PAGE_MIXER) {
    int track = getActiveSequencerTrack();
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d%%", volume); return;
      case 1:
        snprintf(output, outputSize, "%d%%",
                 track == SEQUENCER_TRACK_KICK ? kickLevel : waveVoiceLevels[track]);
        return;
      default:
        if (isActiveSequencerTrackKick()) {
          snprintf(output, outputSize, "%s", isSelectedSequencerStepEnabled() ? "HIT" : "REST");
        } else {
          formatMidiPitch(getSelectedSequencerStepPitch(), output, outputSize);
        }
        return;
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

  if (page == PAGE_KICK_SHAPE) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d ms", kickDecay); return;
      case 1: snprintf(output, outputSize, "%d Hz", kickSweep); return;
      default: snprintf(output, outputSize, "%d%%", kickDrive); return;
    }
  }

  if (page == PAGE_SEQUENCER) {
    switch (encoderIndex) {
      case 0: snprintf(output, outputSize, "%d/%d", sequencerSelectedStep + 1, getSequencerLength()); return;
      case 1:
        if (isActiveSequencerTrackKick()) {
          snprintf(output, outputSize, "%s", isSelectedSequencerStepEnabled() ? "HIT" : "REST");
        } else {
          formatMidiPitch(getSelectedSequencerStepPitch(), output, outputSize);
        }
        return;
      default: snprintf(output, outputSize, "%s", isSelectedSequencerStepEnabled() ? "ACTIVE" : "REST"); return;
    }
  }

  switch (encoderIndex) {
    case 0: snprintf(output, outputSize, "%d", getSequencerLength()); return;
    case 1: snprintf(output, outputSize, "%d", sequencerBpm); return;
    default: snprintf(output, outputSize, "%d/%d", sequencerSelectedStep + 1, getSequencerLength()); return;
  }
}

bool isOscillatorSummaryPage(uint8_t page) {
  return page % PAGE_COUNT == PAGE_TUNING;
}

void formatOscillatorSummaryTitle(char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  snprintf(output, outputSize, "OSC %d  STEP %02d",
           getActiveSequencerTrackVoice() + 1,
           sequencerSelectedStep + 1);
}

void formatOscillatorSummaryLine(uint8_t line, char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  int voice = getActiveSequencerTrackVoice();
  char pitch[8];
  char result[20];

  switch (line) {
    case 0:
      snprintf(output, outputSize, "Wave        %s", waveformNames[waveVoiceShape[voice]]);
      break;
    case 1:
      formatMidiPitch(getSelectedSequencerStepPitch(), pitch, sizeof(pitch));
      snprintf(output, outputSize, "Pitch       %s", pitch);
      break;
    case 2:
      snprintf(output, outputSize, "Tune        %+d st", waveVoiceTune[voice]);
      break;
    case 3:
      snprintf(output, outputSize, "Fine        %+d ct", waveVoiceDetune[voice]);
      break;
    case 4:
      formatSequencerResultPitch(voice, sequencerSelectedStep, result, sizeof(result));
      snprintf(output, outputSize, "Result      %s", result);
      break;
    default:
      snprintf(output, outputSize, "Frequency   %.2f Hz",
               getSequencerResultFrequency(voice, sequencerSelectedStep));
      break;
  }
}

void adjustParameter(uint8_t page, uint8_t encoderIndex, int delta) {
  if (delta == 0) return;

  page %= PAGE_COUNT;

  if (page == PAGE_VOICE_1 || page == PAGE_VOICE_2) {
    encoderIndex %= 4;
    int voice = page == PAGE_VOICE_1 ? 0 : 1;
    switch (encoderIndex) {
      case 0:
        setWaveVoiceShapeForTrack(voice, waveVoiceShape[voice] + delta);
        break;
      case 1:
        waveVoiceTune[voice] = clampInt(waveVoiceTune[voice] + delta, -24, 24);
        break;
      case 2:
        waveVoiceFold[voice] = clampInt(waveVoiceFold[voice] + delta * 2, 0, 100);
        break;
      default:
        waveVoiceDetune[voice] = clampInt(waveVoiceDetune[voice] + delta, -50, 50);
        break;
    }
  } else if (page == PAGE_KICK) {
    switch (encoderIndex % 3) {
      case 0:
        kickLevel = clampInt(kickLevel + delta * 5, 0, 100);
        if (kickLevel > 0) triggerKickVoice();
        break;
      case 1: kickPitch = clampInt(kickPitch + delta, 25, 120); break;
      default: kickAttack = clampInt(kickAttack + delta * 5, 0, 100); break;
    }
  } else if (page == PAGE_TUNING) {
    encoderIndex %= 3;
    int voice = getActiveSequencerTrackVoice();
    switch (encoderIndex) {
      case 0:
        sequencerStepPitches[voice][sequencerSelectedStep] = clampMidiPitch(
          sequencerStepPitches[voice][sequencerSelectedStep] + delta
        );
        sequencerStepEnabled[voice][sequencerSelectedStep] = true;
        break;
      case 1: waveVoiceTune[voice] = clampInt(waveVoiceTune[voice] + delta, -24, 24); break;
      default: waveVoiceDetune[voice] = clampInt(waveVoiceDetune[voice] + delta, -50, 50); break;
    }
  } else if (page == PAGE_ENVELOPE) {
    encoderIndex %= 3;
    switch (encoderIndex) {
      case 0: attack = clampInt(attack + delta, 0, 500); break;
      case 1: decay = clampInt(decay + delta * 5, 0, 2000); break;
      default: sustain = clampInt(sustain + delta, 0, 100); break;
    }
  } else if (page == PAGE_ENVELOPE_CLOCK) {
    encoderIndex %= 3;
    switch (encoderIndex) {
      case 0: releaseMs = clampInt(releaseMs + delta * 20, 0, 3000); break;
      case 1: sequencerBpm = clampInt(sequencerBpm + delta * 2, 40, 240); break;
      default:
        sequencerLength = clampInt(sequencerLength + delta, 1, SEQUENCER_STEP_COUNT);
        clampSequencerIndexes();
        if (!transportRunning) resetSequencerClock();
        break;
    }
  } else if (page == PAGE_MIXER) {
    encoderIndex %= 3;
    int track = getActiveSequencerTrack();
    switch (encoderIndex) {
      case 0: volume = clampInt(volume + delta, 0, 100); break;
      case 1:
        if (track == SEQUENCER_TRACK_KICK) {
          kickLevel = clampInt(kickLevel + delta * 5, 0, 100);
        } else {
          waveVoiceLevels[track] = clampInt(waveVoiceLevels[track] + delta * 5, 0, 100);
        }
        break;
      default:
        if (isActiveSequencerTrackKick()) {
          kickStepEnabled[sequencerSelectedStep] = delta > 0;
        } else {
          int voice = getActiveSequencerTrackVoice();
          sequencerStepPitches[voice][sequencerSelectedStep] = clampMidiPitch(
            sequencerStepPitches[voice][sequencerSelectedStep] + delta
          );
          sequencerStepEnabled[voice][sequencerSelectedStep] = true;
        }
        break;
    }
  } else if (page == PAGE_FILTER_FX) {
    encoderIndex %= 3;
    switch (encoderIndex) {
      case 0: filterCutoff = clampInt(filterCutoff + delta * 50, FILTER_MIN_CUTOFF_HZ, FILTER_MAX_CUTOFF_HZ); break;
      case 1: filterType = wrapIndex(filterType + delta, FILTER_TYPE_COUNT); break;
      default: filterEnabled = !filterEnabled; break;
    }
  } else if (page == PAGE_DELAY_REVERB) {
    encoderIndex %= 3;
    switch (encoderIndex) {
      case 0: delayMix = clampInt(delayMix + delta * 5, 0, 100); break;
      case 1: delayFeedback = clampInt(delayFeedback + delta * 5, 0, 90); break;
      default: reverbMix = clampInt(reverbMix + delta * 5, 0, 100); break;
    }
  } else if (page == PAGE_KICK_SHAPE) {
    encoderIndex %= 3;
    switch (encoderIndex) {
      case 0: kickDecay = clampInt(kickDecay + delta * 10, 80, 1200); break;
      case 1: kickSweep = clampInt(kickSweep + delta * 5, 0, 260); break;
      default: kickDrive = clampInt(kickDrive + delta * 5, 0, 100); break;
    }
  } else if (page == PAGE_SEQUENCER) {
    encoderIndex %= 3;
    switch (encoderIndex) {
      case 0:
        sequencerSelectedStep = wrapIndex(sequencerSelectedStep + delta, getSequencerLength());
        if (!transportRunning) {
          sequencerCurrentStep = sequencerSelectedStep;
          resetSequencerClock();
        }
        break;
      case 1:
      {
        if (isActiveSequencerTrackKick()) {
          kickStepEnabled[sequencerSelectedStep] = delta > 0;
          if (kickLevel == 0 && kickStepEnabled[sequencerSelectedStep]) kickLevel = 80;
          if (kickStepEnabled[sequencerSelectedStep]) triggerKickVoice();
        } else {
          int voice = getActiveSequencerTrackVoice();
          sequencerStepPitches[voice][sequencerSelectedStep] = clampMidiPitch(
            sequencerStepPitches[voice][sequencerSelectedStep] + delta
          );
          sequencerStepEnabled[voice][sequencerSelectedStep] = true;
        }
        break;
      }
      default:
        if (isActiveSequencerTrackKick()) {
          kickStepEnabled[sequencerSelectedStep] = delta > 0;
          if (kickLevel == 0 && kickStepEnabled[sequencerSelectedStep]) kickLevel = 80;
          if (kickStepEnabled[sequencerSelectedStep]) triggerKickVoice();
        } else {
          int voice = getActiveSequencerTrackVoice();
          sequencerStepEnabled[voice][sequencerSelectedStep] = delta > 0;
        }
        break;
    }
  } else {
    encoderIndex %= 3;
    switch (encoderIndex) {
      case 0:
        sequencerLength = clampInt(sequencerLength + delta, 1, SEQUENCER_STEP_COUNT);
        clampSequencerIndexes();
        if (!transportRunning) resetSequencerClock();
        break;
      case 1:
        sequencerBpm = clampInt(sequencerBpm + delta * 2, 40, 240);
        break;
      default:
        sequencerSelectedStep = wrapIndex(sequencerSelectedStep + delta, getSequencerLength());
        if (!transportRunning) {
          sequencerCurrentStep = sequencerSelectedStep;
          resetSequencerClock();
        }
        break;
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
    if (isActiveSequencerTrackKick()) {
      kickStepEnabled[sequencerSelectedStep] = !kickStepEnabled[sequencerSelectedStep];
      if (kickLevel == 0 && kickStepEnabled[sequencerSelectedStep]) kickLevel = 80;
      if (kickStepEnabled[sequencerSelectedStep]) triggerKickVoice();
    } else {
      int voice = getActiveSequencerTrackVoice();
      sequencerStepEnabled[voice][sequencerSelectedStep] = !sequencerStepEnabled[voice][sequencerSelectedStep];
    }
  } else if (page == PAGE_TRANSPORT) {
    if (transportRunning) stopTransport(); else startTransport();
  }

  valueDirty = true;
  fullRedraw = false;
}

uint32_t stateHash() {
  uint32_t hash = 2166136261u;

  const int values[] = {
    waveVoiceShape[0], waveVoiceShape[1], waveVoiceTune[0], waveVoiceTune[1],
    waveVoiceFold[0], waveVoiceFold[1], waveVoiceDetune[0], waveVoiceDetune[1],
    waveVoiceLevels[0], waveVoiceLevels[1], frequency,
    attack, decay, sustain, releaseMs, volume, kickLevel,
    kickPitch, kickDecay, kickSweep, kickAttack, kickDrive,
    filterCutoff, filterType, filterEnabled ? 1 : 0,
    delayMix, delayFeedback, reverbMix,
    sequencerBpm, activeSequencerTrack, activeSequencerVoice, sequencerSelectedStep, sequencerLength,
    getSelectedSequencerStepPitch(), isSelectedSequencerStepEnabled() ? 1 : 0,
    transportRunning ? 1 : 0
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    hash ^= (uint32_t)values[i];
    hash *= 16777619u;
  }

  for (int step = 0; step < SEQUENCER_STEP_COUNT; step++) {
    hash ^= kickStepEnabled[step] ? 0x9E3779B9u + (uint32_t)step : (uint32_t)step;
    hash *= 16777619u;
  }

  return hash;
}

bool isTransportRunning() {
  return transportRunning;
}

}  // namespace ControlSurfaceBridge
