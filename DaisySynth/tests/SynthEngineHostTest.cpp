#include "SynthEngine.h"

#include <math.h>
#include <stdio.h>

namespace {

bool Expect(bool condition, const char* message) {
  if (condition) return true;
  fprintf(stderr, "FAIL: %s\n", message);
  return false;
}

PolyDsp::VoicePatch MakeWorstCasePatch() {
  PolyDsp::VoicePatch patch = PolyDsp::MakeDefaultVoicePatch();
  patch.voice.filterEnabled = true;
  patch.voice.filterMode = PolyDsp::FilterMode::LowPass;
  patch.voice.filterCutoffHz = 5200.0f;
  patch.voice.filterResonance = 0.3f;
  patch.voice.noiseLevel = 0.05f;

  for (size_t oscillator = 0;
       oscillator < PolyDsp::kOscillatorsPerVoice;
       ++oscillator) {
    patch.oscillators[oscillator].enabled = true;
    patch.oscillators[oscillator].waveform = static_cast<uint8_t>(oscillator);
    patch.oscillators[oscillator].level = 0.28f;
    patch.oscillators[oscillator].semitoneTune =
      static_cast<int8_t>(oscillator * 7);
  }

  for (size_t envelope = 0;
       envelope < PolyDsp::kEnvelopesPerVoice;
       ++envelope) {
    patch.envelopes[envelope].enabled = true;
    patch.envelopes[envelope].attackSeconds = 0.0f;
    patch.envelopes[envelope].decaySeconds = 0.08f;
    patch.envelopes[envelope].sustain = 0.75f;
    patch.envelopes[envelope].releaseSeconds = 0.02f;
  }

  for (size_t lfo = 0; lfo < PolyDsp::kLfosPerVoice; ++lfo) {
    patch.lfos[lfo].enabled = true;
    patch.lfos[lfo].waveform = lfo == 0
                                 ? PolyDsp::LfoWaveform::Sine
                                 : PolyDsp::LfoWaveform::Triangle;
    patch.lfos[lfo].frequencyHz = 3.0f + lfo;
    patch.lfos[lfo].amount = 0.5f;
  }

  patch.routes[1] = {
    true,
    PolyDsp::ModSource::Env2,
    PolyDsp::ModDestination::FilterCutoff,
    0.25f,
    PolyDsp::ModCurve::Linear,
    PolyDsp::ModulationScope::PerVoice,
  };
  patch.routes[2] = {
    true,
    PolyDsp::ModSource::Env3,
    PolyDsp::ModDestination::Osc3Wavefold,
    0.4f,
    PolyDsp::ModCurve::Linear,
    PolyDsp::ModulationScope::PerVoice,
  };
  patch.routes[3] = {
    true,
    PolyDsp::ModSource::Lfo1,
    PolyDsp::ModDestination::AllOscPitch,
    0.04f,
    PolyDsp::ModCurve::Linear,
    PolyDsp::ModulationScope::PerVoice,
  };
  patch.routes[4] = {
    true,
    PolyDsp::ModSource::Lfo2,
    PolyDsp::ModDestination::NoiseLevel,
    0.1f,
    PolyDsp::ModCurve::Linear,
    PolyDsp::ModulationScope::PerVoice,
  };
  patch.routes[5] = {
    true,
    PolyDsp::ModSource::Lfo1,
    PolyDsp::ModDestination::DelayMix,
    0.2f,
    PolyDsp::ModCurve::Linear,
    PolyDsp::ModulationScope::Global,
  };
  patch.routes[6] = {
    true,
    PolyDsp::ModSource::Lfo2,
    PolyDsp::ModDestination::ReverbMix,
    0.2f,
    PolyDsp::ModCurve::Linear,
    PolyDsp::ModulationScope::Global,
  };
  return patch;
}

}  // namespace

bool TestIndependentSequencerVoices() {
  PolyDsp::SynthEngine engine;
  engine.Init(48000.0f);
  engine.ReserveSequencerVoices(true);
  engine.TriggerSequencerVoice(0, 48, 127);
  engine.TriggerSequencerVoice(1, 72, 127);
  bool passed = Expect(engine.GetVoice(0).MidiNote() == 48 &&
                       engine.GetVoice(1).MidiNote() == 72,
                       "V1/V2 must play independent notes");
  engine.SetSequencerGate(0, false);
  passed = Expect(!engine.GetVoice(0).GateState() && engine.GetVoice(1).GateState(),
                  "a rest in V1 must not close V2") && passed;
  engine.TriggerSequencerVoice(0, 55, 127);
  passed = Expect(engine.GetVoice(1).MidiNote() == 72 && engine.GetVoice(1).GateState(),
                  "retriggering V1 must leave V2 playing its own note") && passed;
  engine.NoteOff(72);
  passed = Expect(engine.GetVoice(1).GateState(),
                  "MIDI note-off must not release a reserved sequencer voice") && passed;
  for (int note = 60; note < 68; ++note) engine.NoteOn(note, 100);
  passed = Expect(engine.GetVoice(0).MidiNote() == 55 && engine.GetVoice(1).MidiNote() == 72,
                  "polyphonic allocation must reserve both sequencer voices") && passed;
  engine.ReserveSequencerVoices(false);
  passed = Expect(!engine.GetVoice(0).GateState() && !engine.GetVoice(1).GateState(),
                  "stopping must release both sequencer voices") && passed;

  // Each of V2's three oscillators must produce the same audio even when the
  // matching oscillator in V1 is changed. Compare independent engine instances.
  for (size_t oscillator = 0; oscillator < PolyDsp::kOscillatorsPerVoice; ++oscillator) {
    PolyDsp::SynthEngine reference, edited;
    reference.Init(48000.0f);
    edited.Init(48000.0f);
    auto patch = PolyDsp::MakeDefaultVoicePatch();
    for (size_t i = 0; i < PolyDsp::kOscillatorsPerVoice; ++i) {
      patch.oscillators[i].enabled = i == oscillator;
      patch.oscillators[i].level = 0.5f;
    }
    reference.ApplyVoicePatch(1, patch);
    edited.ApplyVoicePatch(1, patch);
    edited.SetOscillatorParameter(0, oscillator, PolyDsp::OscillatorParameter::Waveform, 3);
    edited.SetOscillatorParameter(0, oscillator, PolyDsp::OscillatorParameter::SemitoneTune, 12);
    reference.TriggerSequencerVoice(1, 60, 127);
    edited.TriggerSequencerVoice(1, 60, 127);
    float peak = 0.0f;
    for (int frame = 0; frame < 4096; ++frame) {
      float refLeft, refRight, editLeft, editRight;
      reference.Process(refLeft, refRight);
      edited.Process(editLeft, editRight);
      if (!Expect(refLeft == editLeft && refRight == editRight,
                  "editing V1 must not change V2 oscillator audio")) return false;
      peak = fmaxf(peak, fmaxf(fabsf(editLeft), fabsf(editRight)));
    }
    passed = Expect(peak > 0.00001f, "each V2 oscillator must be audible") && passed;
  }
  return passed;
}

int main() {
  constexpr float kSampleRate = 48000.0f;
  PolyDsp::VoiceOscillator waveformProbe;
  waveformProbe.Init(kSampleRate, 0x12345678U);
  waveformProbe.SetFreq(173.0f);
  waveformProbe.SetPw(0.31f);
  float waveformSignatures[16] = {};
  bool passed = true;
  for (uint8_t waveform = 0; waveform < 16; ++waveform) {
    waveformProbe.SetWaveform(waveform);
    waveformProbe.Reset(0.0f);
    float peak = 0.0f;
    float signature = 0.0f;
    for (size_t frame = 0; frame < 1024; ++frame) {
      float sample = waveformProbe.Process();
      passed = Expect(isfinite(sample), "waveform produced non-finite output") && passed;
      peak = fmaxf(peak, fabsf(sample));
      signature += sample * static_cast<float>((frame % 29) + 1);
    }
    passed = Expect(peak > 0.00001f, "waveform produced silence") && passed;
    waveformSignatures[waveform] = signature;
  }
  passed = Expect(fabsf(waveformSignatures[8] - waveformSignatures[0]) > 0.01f,
                  "EVEN still aliases the sine fallback") && passed;
  passed = Expect(fabsf(waveformSignatures[9] - waveformSignatures[0]) > 0.01f,
                  "ODD still aliases the sine fallback") && passed;
  passed = Expect(fabsf(waveformSignatures[15] - waveformSignatures[0]) > 0.01f,
                  "FORMANT still aliases the sine fallback") && passed;

  // Match the firmware's static-storage engine. DaisySP's Oscillator keeps
  // internal integration history that is zero-initialized by static storage.
  static PolyDsp::SynthEngine engine;
  engine.Init(kSampleRate);
  PolyDsp::VoicePatch patch = MakeWorstCasePatch();
  for (size_t voice = 0; voice < PolyDsp::kMaxVoices; ++voice) {
    engine.ApplyVoicePatch(voice, patch);
  }

  const uint8_t notes[PolyDsp::kMaxVoices] = {60, 64, 67, 71};
  for (size_t voice = 0; voice < PolyDsp::kMaxVoices; ++voice) {
    engine.NoteOn(notes[voice], static_cast<uint8_t>(127 - voice * 8));
  }

  passed = Expect(engine.ActiveVoiceCount() == PolyDsp::kMaxVoices,
                  "four simultaneous voices were not allocated") && passed;
  for (size_t voice = 0; voice < PolyDsp::kMaxVoices; ++voice) {
    passed = Expect(engine.GetVoice(voice).MidiNote() == notes[voice],
                    "first-inactive allocation order changed") && passed;
  }

  float peak = 0.0f;
  bool globalModulationObserved = false;
  for (size_t frame = 0; frame < 4096; ++frame) {
    float left = 0.0f;
    float right = 0.0f;
    engine.Process(left, right);
    passed = Expect(isfinite(left) && isfinite(right),
                    "non-finite polyphonic output") && passed;
    peak = fmaxf(peak, fmaxf(fabsf(left), fabsf(right)));
    const PolyDsp::GlobalModulation& global = engine.GetGlobalModulation();
    globalModulationObserved = globalModulationObserved ||
                               fabsf(global.delayMix) > 0.000001f ||
                               fabsf(global.reverbMix) > 0.000001f;
  }
  passed = Expect(peak > 0.00001f, "four voices produced silence") && passed;
  passed = Expect(globalModulationObserved,
                  "global delay/reverb modulation was not produced") && passed;

  engine.NoteOn(76, 120);
  passed = Expect(engine.ActiveVoiceCount() == PolyDsp::kMaxVoices,
                  "voice stealing changed the active voice count") && passed;
  passed = Expect(engine.GetVoice(0).MidiNote() == 76,
                  "oldest active voice was not stolen") && passed;

  engine.AllNotesOff();
  for (size_t frame = 0; frame < 12000; ++frame) {
    float left = 0.0f;
    float right = 0.0f;
    engine.Process(left, right);
  }
  passed = Expect(engine.ActiveVoiceCount() == 0,
                  "released voices did not become inactive") && passed;

  // Exercise the exact reserved voice path used by the UART sequencer. A
  // note followed by a rest and a second note must keep producing finite
  // output and must not leave the compatibility voice stuck inactive.
  engine.ReserveCompatibilityVoice(true);
  engine.TriggerCompatibilityVoice(57, 127, false);
  engine.SetCompatibilityOscillatorNote(0, 57.0f);
  engine.SetCompatibilityOscillatorGate(0, true);
  engine.SetCompatibilityOscillatorGate(1, false);
  engine.SetCompatibilityOscillatorGate(2, false);

  float compatibilityPeak = 0.0f;
  for (size_t frame = 0; frame < 4096; ++frame) {
    float left = 0.0f;
    float right = 0.0f;
    engine.Process(left, right);
    passed = Expect(isfinite(left) && isfinite(right),
                    "compatibility sequencer note produced non-finite output") && passed;
    compatibilityPeak = fmaxf(
      compatibilityPeak, fmaxf(fabsf(left), fabsf(right))
    );
  }
  passed = Expect(compatibilityPeak > 0.00001f,
                  "compatibility sequencer note produced silence") && passed;

  engine.SetCompatibilityGate(false);
  for (size_t frame = 0; frame < 12000; ++frame) {
    float left = 0.0f;
    float right = 0.0f;
    engine.Process(left, right);
  }
  passed = Expect(engine.ActiveVoiceCount() == 0,
                  "compatibility sequencer rest did not release its voice") && passed;

  engine.TriggerCompatibilityVoice(64, 127, false);
  engine.SetCompatibilityOscillatorNote(0, 64.0f);
  engine.SetCompatibilityOscillatorGate(0, true);
  engine.SetCompatibilityOscillatorGate(1, false);
  engine.SetCompatibilityOscillatorGate(2, false);
  float retriggerPeak = 0.0f;
  for (size_t frame = 0; frame < 4096; ++frame) {
    float left = 0.0f;
    float right = 0.0f;
    engine.Process(left, right);
    passed = Expect(isfinite(left) && isfinite(right),
                    "compatibility sequencer retrigger produced non-finite output") && passed;
    retriggerPeak = fmaxf(retriggerPeak, fmaxf(fabsf(left), fabsf(right)));
  }
  passed = Expect(retriggerPeak > 0.00001f,
                  "compatibility sequencer did not recover after a rest") && passed;

  passed = TestIndependentSequencerVoices() && passed;
  if (!passed) return 1;
  printf("PASS: 16 waveforms, polyphony, modulation, independent V1/V2 oscillators and sequencer note/rest/retrigger (peaks %.4f/%.4f)\n",
         peak,
         retriggerPeak);
  return 0;
}
