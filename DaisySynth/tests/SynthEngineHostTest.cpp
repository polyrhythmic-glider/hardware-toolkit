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

  if (!passed) return 1;
  printf("PASS: 16 waveforms, 4 voices, 12 oscillators, modulation, release and oldest-voice stealing (peak %.4f)\n",
         peak);
  return 0;
}
