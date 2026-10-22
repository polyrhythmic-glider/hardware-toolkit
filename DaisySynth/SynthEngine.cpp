#include "SynthEngine.h"

#include <math.h>

namespace PolyDsp {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinFrequencyHz = 0.01f;
constexpr float kMinFilterCutoffHz = 20.0f;
constexpr float kMaxPitch = 127.0f;
constexpr float kMinPitch = 0.0f;
constexpr uint8_t kControlRatePeriod = 8;

float Clamp(float value, float minimum, float maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}

size_t DestinationIndex(ModDestination destination) {
  return static_cast<size_t>(destination);
}

size_t SourceIndex(ModSource source) {
  return static_cast<size_t>(source);
}

ModDestination OscillatorDestination(size_t oscillatorIndex,
                                     ModDestination oscillator1Destination) {
  return static_cast<ModDestination>(
    static_cast<uint8_t>(oscillator1Destination) + static_cast<uint8_t>(oscillatorIndex)
  );
}

float ModValue(
  const std::array<float, static_cast<size_t>(ModDestination::Count)>& modulation,
  ModDestination destination) {
  return modulation[DestinationIndex(destination)];
}

}  // namespace

bool IsGlobalDestination(ModDestination destination) {
  return destination == ModDestination::DelayMix ||
         destination == ModDestination::ReverbMix;
}

VoicePatch MakeDefaultVoicePatch() {
  VoicePatch patch = {};

  patch.voice.filterEnabled = false;
  patch.voice.filterMode = FilterMode::LowPass;
  patch.voice.filterCutoffHz = 2400.0f;
  patch.voice.filterResonance = 0.15f;
  patch.voice.filterDrive = 0.0f;
  patch.voice.amp = 0.0f;
  patch.voice.pan = 0.0f;
  patch.voice.noiseLevel = 0.0f;

  patch.oscillators[0].enabled = true;
  patch.oscillators[0].level = 1.0f;
  patch.oscillators[0].pulseWidth = 0.5f;
  patch.oscillators[0].keyTracking = 1.0f;
  patch.oscillators[0].modulationInputAmount = 1.0f;
  for (size_t oscillator = 1; oscillator < kOscillatorsPerVoice; ++oscillator) {
    patch.oscillators[oscillator].enabled = false;
    patch.oscillators[oscillator].level = 0.0f;
    patch.oscillators[oscillator].pulseWidth = 0.5f;
    patch.oscillators[oscillator].keyTracking = 1.0f;
    patch.oscillators[oscillator].modulationInputAmount = 1.0f;
  }

  patch.envelopes[0].enabled = true;
  patch.envelopes[0].amount = 1.0f;
  patch.envelopes[0].retriggerMode = RetriggerMode::Soft;
  for (size_t envelope = 1; envelope < kEnvelopesPerVoice; ++envelope) {
    patch.envelopes[envelope].enabled = false;
    patch.envelopes[envelope].amount = 1.0f;
    patch.envelopes[envelope].retriggerMode = RetriggerMode::Soft;
  }

  for (size_t lfo = 0; lfo < kLfosPerVoice; ++lfo) {
    patch.lfos[lfo].enabled = false;
    patch.lfos[lfo].amount = 0.0f;
    patch.lfos[lfo].freeRunning = true;
  }

  patch.routes[0].enabled = true;
  patch.routes[0].source = ModSource::Env1;
  patch.routes[0].destination = ModDestination::Amp;
  patch.routes[0].amount = 1.0f;
  patch.routes[0].scope = ModulationScope::PerVoice;
  return patch;
}

void SmoothedParameter::Init(float initialValue, float coefficient) {
  current_ = initialValue;
  target_ = initialValue;
  coefficient_ = Clamp(coefficient, 0.00001f, 1.0f);
}

void SmoothedParameter::SetTarget(float target) {
  target_ = target;
}

void SmoothedParameter::Snap(float value) {
  current_ = value;
  target_ = value;
}

float SmoothedParameter::Process() {
  current_ += (target_ - current_) * coefficient_;
  return current_;
}

void Vca::Init(float smoothingCoefficient) {
  gain_.Init(0.0f, smoothingCoefficient);
}

void Vca::SetGain(float gain) {
  gain_.SetTarget(Clamp(gain, 0.0f, 1.0f));
}

float Vca::Process(float input) {
  return input * gain_.Process();
}

void RandomGenerator::Seed(uint32_t seed) {
  state_ = seed == 0 ? 1 : seed;
}

float RandomGenerator::NextBipolar() {
  state_ = state_ * 1664525U + 1013904223U;
  return static_cast<float>((state_ >> 8) & 0x00FFFFFFU) *
           (2.0f / 16777215.0f) -
         1.0f;
}

void VoiceOscillator::Init(float sampleRate, uint32_t seed) {
  sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;
  random_.Seed(seed);
  base_.Init(sampleRate_);
  base_.SetAmp(1.0f);
  frequencyHz_ = 100.0f;
  waveform_ = 0;
  pulseWidth_ = 0.5f;
  base_.SetWaveform(MapDaisyWaveform(waveform_));
  base_.SetFreq(frequencyHz_);
  base_.SetPw(pulseWidth_);
  Reset();
}

void VoiceOscillator::SetWaveform(uint8_t waveform) {
  waveform_ = waveform;
  base_.SetWaveform(MapDaisyWaveform(waveform_));
}

void VoiceOscillator::SetFreq(float frequencyHz) {
  frequencyHz_ = Clamp(frequencyHz, kMinFrequencyHz, sampleRate_ * 0.45f);
  base_.SetFreq(frequencyHz_);
}

void VoiceOscillator::SetPw(float pulseWidth) {
  pulseWidth_ = Clamp(pulseWidth, 0.01f, 0.99f);
  base_.SetPw(pulseWidth_);
}

void VoiceOscillator::Reset(float phase) {
  phase_ = WrapPhase(phase);
  subPhase_ = WrapPhase(phase * 0.5f);
  superPhaseLow_ = phase_;
  superPhaseHigh_ = phase_;
  base_.Reset(phase_);
}

float VoiceOscillator::Process() {
  float output = UsesDaisyWaveform(waveform_)
                   ? base_.Process()
                   : RenderExtendedWaveform();
  AdvancePhases();
  return output;
}

uint8_t VoiceOscillator::MapDaisyWaveform(uint8_t waveform) {
  switch (waveform) {
    case 0: return daisysp::Oscillator::WAVE_SIN;
    case 1: return daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
    case 2: return daisysp::Oscillator::WAVE_POLYBLEP_TRI;
    case 3: return daisysp::Oscillator::WAVE_POLYBLEP_SAW;
    case 4: return daisysp::Oscillator::WAVE_RAMP;
    case 5:
    case 11: return daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
    default: return daisysp::Oscillator::WAVE_SIN;
  }
}

bool VoiceOscillator::UsesDaisyWaveform(uint8_t waveform) {
  return waveform <= 5 || waveform == 11;
}

float VoiceOscillator::WrapPhase(float phase) {
  phase -= floorf(phase);
  return phase < 0.0f ? phase + 1.0f : phase;
}

float VoiceOscillator::Sine(float phase) {
  return sinf(WrapPhase(phase) * 2.0f * kPi);
}

float VoiceOscillator::Saw(float phase) {
  return WrapPhase(phase) * 2.0f - 1.0f;
}

void VoiceOscillator::AdvancePhases() {
  float increment = frequencyHz_ / sampleRate_;
  phase_ = WrapPhase(phase_ + increment);
  subPhase_ = WrapPhase(subPhase_ + increment * 0.5f);
  superPhaseLow_ = WrapPhase(superPhaseLow_ + increment * 0.987f);
  superPhaseHigh_ = WrapPhase(superPhaseHigh_ + increment * 1.013f);
}

float VoiceOscillator::RenderExtendedWaveform() {
  switch (waveform_) {
    case 6:  // SUB: fundamental plus a true octave-down component.
      return Sine(phase_) * 0.65f + Sine(subPhase_) * 0.35f;
    case 7:  // ORGAN
      return Sine(phase_) * 0.58f +
             Sine(phase_ * 2.0f) * 0.28f +
             Sine(phase_ * 3.0f) * 0.14f;
    case 8:  // EVEN
      return Sine(phase_) * 0.55f + Sine(phase_ * 2.0f) * 0.45f;
    case 9:  // ODD
      return Sine(phase_) * 0.72f +
             Sine(phase_ * 3.0f) * 0.20f +
             Sine(phase_ * 5.0f) * 0.08f;
    case 10:  // SUPER
      return (Saw(phase_) + Saw(superPhaseLow_) + Saw(superPhaseHigh_)) / 3.0f;
    case 12:  // STAIR
    {
      constexpr float kSteps = 8.0f;
      float step = floorf(Clamp(phase_, 0.0f, 0.999999f) * kSteps);
      return step / (kSteps - 1.0f) * 2.0f - 1.0f;
    }
    case 13:  // CLIP
      return Clamp(Sine(phase_) * 1.8f, -0.8f, 0.8f) / 0.8f;
    case 14:  // NOISE
      return random_.NextBipolar();
    case 15:  // FORMANT
      return Sine(phase_) * 0.52f +
             Sine(phase_ * 3.0f) * 0.30f +
             Sine(phase_ * 7.0f) * 0.18f;
    default:
      return Sine(phase_);
  }
}

void Lfo::Init(float sampleRate, uint32_t seed) {
  sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;
  random_.Seed(seed);
  phase_ = 0.0f;
  heldValue_ = random_.NextBipolar();
}

void Lfo::SetParameters(const LfoParameters& parameters) {
  bool phaseChanged = parameters.phase != parameters_.phase;
  parameters_ = parameters;
  parameters_.frequencyHz = Clamp(parameters_.frequencyHz, 0.0f, sampleRate_ * 0.25f);
  parameters_.phase -= floorf(parameters_.phase);
  parameters_.amount = Clamp(parameters_.amount, -1.0f, 1.0f);
  if (phaseChanged) phase_ = parameters_.phase;
}

void Lfo::Retrigger() {
  phase_ = parameters_.phase;
  heldValue_ = random_.NextBipolar();
}

float Lfo::RenderWaveform(float phase) const {
  switch (parameters_.waveform) {
    case LfoWaveform::Triangle:
      return 1.0f - 4.0f * fabsf(phase - 0.5f);
    case LfoWaveform::Saw:
      return phase * 2.0f - 1.0f;
    case LfoWaveform::Ramp:
      return 1.0f - phase * 2.0f;
    case LfoWaveform::Square:
      return phase < 0.5f ? 1.0f : -1.0f;
    case LfoWaveform::SampleAndHold:
      return heldValue_;
    case LfoWaveform::Sine:
    default:
      return sinf(phase * 2.0f * kPi);
  }
}

float Lfo::Process(bool voiceActive) {
  if (!parameters_.enabled) return 0.0f;
  if (!voiceActive && !parameters_.freeRunning) return 0.0f;

  float output = RenderWaveform(phase_);
  float increment = parameters_.frequencyHz / sampleRate_;
  phase_ += increment;
  if (phase_ >= 1.0f) {
    phase_ -= floorf(phase_);
    if (parameters_.waveform == LfoWaveform::SampleAndHold) {
      heldValue_ = random_.NextBipolar();
    }
  }

  if (!parameters_.bipolar) output = (output + 1.0f) * 0.5f;
  return output * parameters_.amount;
}

void ModulationMatrix::Reset() {
  for (size_t route = 0; route < routes_.size(); ++route) {
    routes_[route] = ModulationRoute{};
  }
}

void ModulationMatrix::SetRoute(size_t index, const ModulationRoute& route) {
  if (index >= routes_.size()) return;
  routes_[index] = route;
  routes_[index].amount = Clamp(routes_[index].amount, -1.0f, 1.0f);
  routes_[index].scope = IsGlobalDestination(routes_[index].destination)
                           ? ModulationScope::Global
                           : ModulationScope::PerVoice;
}

const ModulationRoute& ModulationMatrix::GetRoute(size_t index) const {
  return routes_[index < routes_.size() ? index : 0];
}

void ModulationMatrix::Evaluate(
  const std::array<float, static_cast<size_t>(ModSource::Count)>& sources,
  std::array<float, static_cast<size_t>(ModDestination::Count)>& destinations) const {
  destinations.fill(0.0f);
  for (size_t routeIndex = 0; routeIndex < routes_.size(); ++routeIndex) {
    const ModulationRoute& route = routes_[routeIndex];
    size_t source = SourceIndex(route.source);
    size_t destination = DestinationIndex(route.destination);
    if (!route.enabled || source >= sources.size() || destination >= destinations.size()) continue;

    // Reserved curves intentionally evaluate linearly until a future patch
    // version gives them a stable transfer function.
    destinations[destination] += sources[source] * route.amount;
  }
}

float OscillatorMixer::Process(
  const std::array<float, kOscillatorsPerVoice>& oscillatorSamples,
  const std::array<float, kOscillatorsPerVoice>& oscillatorLevels,
  float noiseSample,
  float noiseLevel) const {
  float mixed = noiseSample * noiseLevel;
  float totalLevel = noiseLevel;
  for (size_t oscillator = 0; oscillator < oscillatorSamples.size(); ++oscillator) {
    mixed += oscillatorSamples[oscillator] * oscillatorLevels[oscillator];
    totalLevel += oscillatorLevels[oscillator];
  }
  if (totalLevel > 1.0f) mixed /= totalLevel;
  return mixed;
}

void Voice::Init(float sampleRate, uint32_t randomSeed) {
  sampleRate_ = sampleRate > 1.0f ? sampleRate : 48000.0f;
  active_ = false;
  gateState_ = false;
  velocity_ = 0;
  midiNote_ = 60;
  allocationAge_ = 0;
  controlRateCounter_ = 0;

  for (size_t oscillator = 0; oscillator < kOscillatorsPerVoice; ++oscillator) {
    oscillators_[oscillator].Init(
      sampleRate_, randomSeed + static_cast<uint32_t>(oscillator * 193U + 17U)
    );
    wavefolders_[oscillator].Init();
    oscillatorNotes_[oscillator] = 60.0f;
    oscillatorGates_[oscillator] = false;
    frequencySmoothers_[oscillator].Init(MidiToFrequency(60.0f), 0.02f);
    levelSmoothers_[oscillator].Init(0.0f, 0.01f);
  }

  noise_.Init(randomSeed);
  filter_.Init(sampleRate_);
  vca_.Init(0.01f);
  for (size_t envelope = 0; envelope < kEnvelopesPerVoice; ++envelope) {
    envelopes_[envelope].Init(sampleRate_);
  }
  for (size_t lfo = 0; lfo < kLfosPerVoice; ++lfo) {
    lfos_[lfo].Init(sampleRate_, randomSeed + static_cast<uint32_t>(lfo * 97U + 1U));
  }

  noiseLevelSmoother_.Init(0.0f, 0.01f);
  panSmoother_.Init(0.0f, 0.01f);
  cutoffSmoother_.Init(2400.0f, 0.02f);
  resonanceSmoother_.Init(0.15f, 0.02f);
  ResetPatchToDefaults();
}

void Voice::ResetPatchToDefaults() {
  ApplyPatch(MakeDefaultVoicePatch());
}

void Voice::ApplyPatch(const VoicePatch& patch) {
  patch_ = patch;
  modulationMatrix_.Reset();

  SetVoiceParameter(VoiceParameter::FilterEnabled,
                    patch.voice.filterEnabled ? 1.0f : 0.0f);
  SetVoiceParameter(VoiceParameter::FilterMode,
                    static_cast<float>(patch.voice.filterMode));
  SetVoiceParameter(VoiceParameter::FilterCutoff, patch.voice.filterCutoffHz);
  SetVoiceParameter(VoiceParameter::FilterResonance, patch.voice.filterResonance);
  SetVoiceParameter(VoiceParameter::FilterDrive, patch.voice.filterDrive);
  SetVoiceParameter(VoiceParameter::Amp, patch.voice.amp);
  SetVoiceParameter(VoiceParameter::Pan, patch.voice.pan);
  SetVoiceParameter(VoiceParameter::NoiseLevel, patch.voice.noiseLevel);

  for (size_t oscillator = 0; oscillator < kOscillatorsPerVoice; ++oscillator) {
    SetOscillatorParameter(oscillator, OscillatorParameter::Enabled,
                           patch.oscillators[oscillator].enabled ? 1.0f : 0.0f);
    SetOscillatorParameter(oscillator, OscillatorParameter::Waveform,
                           patch.oscillators[oscillator].waveform);
    SetOscillatorParameter(oscillator, OscillatorParameter::Level,
                           patch.oscillators[oscillator].level);
    SetOscillatorParameter(oscillator, OscillatorParameter::Octave,
                           patch.oscillators[oscillator].octave);
    SetOscillatorParameter(oscillator, OscillatorParameter::SemitoneTune,
                           patch.oscillators[oscillator].semitoneTune);
    SetOscillatorParameter(oscillator, OscillatorParameter::FineTune,
                           patch.oscillators[oscillator].fineTuneCents);
    SetOscillatorParameter(oscillator, OscillatorParameter::PhaseOffset,
                           patch.oscillators[oscillator].phaseOffset);
    SetOscillatorParameter(oscillator, OscillatorParameter::PulseWidth,
                           patch.oscillators[oscillator].pulseWidth);
    SetOscillatorParameter(oscillator, OscillatorParameter::Wavefold,
                           patch.oscillators[oscillator].wavefold);
    SetOscillatorParameter(oscillator, OscillatorParameter::Pan,
                           patch.oscillators[oscillator].pan);
    SetOscillatorParameter(oscillator, OscillatorParameter::KeyTracking,
                           patch.oscillators[oscillator].keyTracking);
    SetOscillatorParameter(oscillator, OscillatorParameter::ModulationInputAmount,
                           patch.oscillators[oscillator].modulationInputAmount);
  }
  for (size_t envelope = 0; envelope < kEnvelopesPerVoice; ++envelope) {
    SetEnvelopeParameter(envelope, EnvelopeParameter::Enabled,
                         patch.envelopes[envelope].enabled ? 1.0f : 0.0f);
    SetEnvelopeParameter(envelope, EnvelopeParameter::Attack,
                         patch.envelopes[envelope].attackSeconds);
    SetEnvelopeParameter(envelope, EnvelopeParameter::Decay,
                         patch.envelopes[envelope].decaySeconds);
    SetEnvelopeParameter(envelope, EnvelopeParameter::Sustain,
                         patch.envelopes[envelope].sustain);
    SetEnvelopeParameter(envelope, EnvelopeParameter::Release,
                         patch.envelopes[envelope].releaseSeconds);
    SetEnvelopeParameter(envelope, EnvelopeParameter::Amount,
                         patch.envelopes[envelope].amount);
    SetEnvelopeParameter(envelope, EnvelopeParameter::VelocitySensitivity,
                         patch.envelopes[envelope].velocitySensitivity);
    SetEnvelopeParameter(envelope, EnvelopeParameter::RetriggerMode,
                         static_cast<float>(patch.envelopes[envelope].retriggerMode));
  }
  for (size_t lfo = 0; lfo < kLfosPerVoice; ++lfo) {
    SetLfoParameter(lfo, LfoParameter::Enabled,
                    patch.lfos[lfo].enabled ? 1.0f : 0.0f);
    SetLfoParameter(lfo, LfoParameter::Waveform,
                    static_cast<float>(patch.lfos[lfo].waveform));
    SetLfoParameter(lfo, LfoParameter::Frequency, patch.lfos[lfo].frequencyHz);
    SetLfoParameter(lfo, LfoParameter::Phase, patch.lfos[lfo].phase);
    SetLfoParameter(lfo, LfoParameter::Bipolar,
                    patch.lfos[lfo].bipolar ? 1.0f : 0.0f);
    SetLfoParameter(lfo, LfoParameter::Amount, patch.lfos[lfo].amount);
    SetLfoParameter(lfo, LfoParameter::RetriggerOnNoteOn,
                    patch.lfos[lfo].retriggerOnNoteOn ? 1.0f : 0.0f);
    SetLfoParameter(lfo, LfoParameter::FreeRunning,
                    patch.lfos[lfo].freeRunning ? 1.0f : 0.0f);
    SetLfoParameter(lfo, LfoParameter::TempoSync,
                    patch.lfos[lfo].tempoSync ? 1.0f : 0.0f);
    SetLfoParameter(lfo, LfoParameter::TempoDivision,
                    patch.lfos[lfo].tempoDivision);
  }
  for (size_t route = 0; route < kMaxModRoutesPerVoice; ++route) {
    modulationMatrix_.SetRoute(route, patch_.routes[route]);
  }

  cutoffSmoother_.SetTarget(Clamp(patch_.voice.filterCutoffHz,
                                  kMinFilterCutoffHz,
                                  sampleRate_ / 3.0f));
  resonanceSmoother_.SetTarget(Clamp(patch_.voice.filterResonance, 0.0f, 1.0f));
  noiseLevelSmoother_.SetTarget(Clamp(patch_.voice.noiseLevel, 0.0f, 1.0f));
  panSmoother_.SetTarget(Clamp(patch_.voice.pan, -1.0f, 1.0f));
  filter_.SetDrive(Clamp(patch_.voice.filterDrive, 0.0f, 1.0f));
}

void Voice::NoteOn(uint8_t note, uint8_t velocity, uint32_t allocationAge) {
  bool wasGated = gateState_;
  active_ = true;
  midiNote_ = note;
  velocity_ = velocity;
  allocationAge_ = allocationAge;

  for (size_t oscillator = 0; oscillator < kOscillatorsPerVoice; ++oscillator) {
    SetOscillatorNote(oscillator, static_cast<float>(note));
    oscillatorGates_[oscillator] = patch_.oscillators[oscillator].enabled;
  }
  for (size_t lfo = 0; lfo < kLfosPerVoice; ++lfo) {
    if (patch_.lfos[lfo].retriggerOnNoteOn || !patch_.lfos[lfo].freeRunning) {
      lfos_[lfo].Retrigger();
    }
  }

  for (size_t envelope = 0; envelope < kEnvelopesPerVoice; ++envelope) {
    if (!patch_.envelopes[envelope].enabled) continue;
    RetriggerMode mode = patch_.envelopes[envelope].retriggerMode;
    if (mode == RetriggerMode::Legato && wasGated) continue;
    envelopes_[envelope].Retrigger(mode == RetriggerMode::Hard);
  }
  gateState_ = true;
  controlRateCounter_ = kControlRatePeriod - 1;
}

void Voice::NoteOff() {
  gateState_ = false;
}

void Voice::AllNotesOff(bool hard) {
  gateState_ = false;
  velocity_ = 0;
  if (hard) {
    for (size_t envelope = 0; envelope < kEnvelopesPerVoice; ++envelope) {
      envelopes_[envelope].Init(sampleRate_);
      RefreshEnvelope(envelope);
    }
    for (size_t oscillator = 0; oscillator < kOscillatorsPerVoice; ++oscillator) {
      oscillatorGates_[oscillator] = false;
    }
    active_ = false;
  }
}

void Voice::Retrigger(bool hard) {
  if (!active_) active_ = true;
  for (size_t envelope = 0; envelope < kEnvelopesPerVoice; ++envelope) {
    if (patch_.envelopes[envelope].enabled) envelopes_[envelope].Retrigger(hard);
  }
}

void Voice::SetGate(bool gate) {
  gateState_ = gate;
}

void Voice::SetVoiceParameter(VoiceParameter parameter, float value) {
  switch (parameter) {
    case VoiceParameter::FilterEnabled:
      patch_.voice.filterEnabled = value >= 0.5f;
      break;
    case VoiceParameter::FilterMode:
      patch_.voice.filterMode = static_cast<FilterMode>(
        static_cast<uint8_t>(Clamp(value, 0.0f, 2.0f))
      );
      break;
    case VoiceParameter::FilterCutoff:
      patch_.voice.filterCutoffHz = Clamp(value, kMinFilterCutoffHz, sampleRate_ / 3.0f);
      cutoffSmoother_.SetTarget(patch_.voice.filterCutoffHz);
      break;
    case VoiceParameter::FilterResonance:
      patch_.voice.filterResonance = Clamp(value, 0.0f, 1.0f);
      resonanceSmoother_.SetTarget(patch_.voice.filterResonance);
      break;
    case VoiceParameter::FilterDrive:
      patch_.voice.filterDrive = Clamp(value, 0.0f, 1.0f);
      filter_.SetDrive(patch_.voice.filterDrive);
      break;
    case VoiceParameter::Amp:
      patch_.voice.amp = Clamp(value, 0.0f, 1.0f);
      break;
    case VoiceParameter::Pan:
      patch_.voice.pan = Clamp(value, -1.0f, 1.0f);
      break;
    case VoiceParameter::NoiseLevel:
      patch_.voice.noiseLevel = Clamp(value, 0.0f, 1.0f);
      break;
  }
}

void Voice::SetOscillatorParameter(size_t oscillatorIndex,
                                   OscillatorParameter parameter,
                                   float value) {
  if (oscillatorIndex >= kOscillatorsPerVoice) return;
  OscillatorParameters& oscillator = patch_.oscillators[oscillatorIndex];
  switch (parameter) {
    case OscillatorParameter::Enabled:
      oscillator.enabled = value >= 0.5f;
      if (active_) oscillatorGates_[oscillatorIndex] = oscillator.enabled;
      break;
    case OscillatorParameter::Waveform:
      oscillator.waveform = static_cast<uint8_t>(Clamp(value, 0.0f, 255.0f));
      oscillators_[oscillatorIndex].SetWaveform(oscillator.waveform);
      break;
    case OscillatorParameter::Level:
      oscillator.level = Clamp(value, 0.0f, 1.0f);
      break;
    case OscillatorParameter::Octave:
      oscillator.octave = static_cast<int8_t>(Clamp(value, -4.0f, 4.0f));
      break;
    case OscillatorParameter::SemitoneTune:
      oscillator.semitoneTune = static_cast<int8_t>(Clamp(value, -48.0f, 48.0f));
      break;
    case OscillatorParameter::FineTune:
      oscillator.fineTuneCents = Clamp(value, -100.0f, 100.0f);
      break;
    case OscillatorParameter::PhaseOffset:
    {
      float phaseOffset = value - floorf(value);
      if (phaseOffset != oscillator.phaseOffset) {
        oscillator.phaseOffset = phaseOffset;
        oscillators_[oscillatorIndex].Reset(oscillator.phaseOffset);
      }
      break;
    }
    case OscillatorParameter::PulseWidth:
      oscillator.pulseWidth = Clamp(value, 0.01f, 0.99f);
      oscillators_[oscillatorIndex].SetPw(oscillator.pulseWidth);
      break;
    case OscillatorParameter::Wavefold:
      oscillator.wavefold = Clamp(value, 0.0f, 1.0f);
      wavefolders_[oscillatorIndex].SetGain(1.0f + oscillator.wavefold * 3.0f);
      break;
    case OscillatorParameter::Pan:
      oscillator.pan = Clamp(value, -1.0f, 1.0f);
      break;
    case OscillatorParameter::KeyTracking:
      oscillator.keyTracking = Clamp(value, 0.0f, 1.0f);
      break;
    case OscillatorParameter::ModulationInputAmount:
      oscillator.modulationInputAmount = Clamp(value, -2.0f, 2.0f);
      break;
  }
}

void Voice::SetEnvelopeParameter(size_t envelopeIndex,
                                 EnvelopeParameter parameter,
                                 float value) {
  if (envelopeIndex >= kEnvelopesPerVoice) return;
  EnvelopeParameters& envelope = patch_.envelopes[envelopeIndex];
  switch (parameter) {
    case EnvelopeParameter::Enabled: envelope.enabled = value >= 0.5f; break;
    case EnvelopeParameter::Attack: envelope.attackSeconds = Clamp(value, 0.0f, 30.0f); break;
    case EnvelopeParameter::Decay: envelope.decaySeconds = Clamp(value, 0.0f, 30.0f); break;
    case EnvelopeParameter::Sustain: envelope.sustain = Clamp(value, 0.0f, 1.0f); break;
    case EnvelopeParameter::Release: envelope.releaseSeconds = Clamp(value, 0.0f, 30.0f); break;
    case EnvelopeParameter::Amount: envelope.amount = Clamp(value, -1.0f, 1.0f); break;
    case EnvelopeParameter::VelocitySensitivity:
      envelope.velocitySensitivity = Clamp(value, 0.0f, 1.0f);
      break;
    case EnvelopeParameter::RetriggerMode:
      envelope.retriggerMode = static_cast<RetriggerMode>(
        static_cast<uint8_t>(Clamp(value, 0.0f, 2.0f))
      );
      break;
  }
  RefreshEnvelope(envelopeIndex);
}

void Voice::SetLfoParameter(size_t lfoIndex, LfoParameter parameter, float value) {
  if (lfoIndex >= kLfosPerVoice) return;
  LfoParameters& lfo = patch_.lfos[lfoIndex];
  switch (parameter) {
    case LfoParameter::Enabled: lfo.enabled = value >= 0.5f; break;
    case LfoParameter::Waveform:
      lfo.waveform = static_cast<LfoWaveform>(static_cast<uint8_t>(Clamp(value, 0.0f, 5.0f)));
      break;
    case LfoParameter::Frequency: lfo.frequencyHz = Clamp(value, 0.0f, 200.0f); break;
    case LfoParameter::Phase: lfo.phase = value - floorf(value); break;
    case LfoParameter::Bipolar: lfo.bipolar = value >= 0.5f; break;
    case LfoParameter::Amount: lfo.amount = Clamp(value, -1.0f, 1.0f); break;
    case LfoParameter::RetriggerOnNoteOn: lfo.retriggerOnNoteOn = value >= 0.5f; break;
    case LfoParameter::FreeRunning: lfo.freeRunning = value >= 0.5f; break;
    case LfoParameter::TempoSync: lfo.tempoSync = value >= 0.5f; break;
    case LfoParameter::TempoDivision:
      lfo.tempoDivision = static_cast<uint8_t>(Clamp(value, 0.0f, 31.0f));
      break;
  }
  RefreshLfo(lfoIndex);
}

void Voice::SetModulationRoute(size_t routeIndex, const ModulationRoute& route) {
  if (routeIndex >= kMaxModRoutesPerVoice) return;
  patch_.routes[routeIndex] = route;
  patch_.routes[routeIndex].amount = Clamp(route.amount, -1.0f, 1.0f);
  patch_.routes[routeIndex].scope = IsGlobalDestination(route.destination)
                                            ? ModulationScope::Global
                                            : ModulationScope::PerVoice;
  modulationMatrix_.SetRoute(routeIndex, patch_.routes[routeIndex]);
}

void Voice::SetOscillatorNote(size_t oscillatorIndex, float midiNote) {
  if (oscillatorIndex >= kOscillatorsPerVoice) return;
  oscillatorNotes_[oscillatorIndex] = Clamp(midiNote, kMinPitch, kMaxPitch);
  const OscillatorParameters& parameters = patch_.oscillators[oscillatorIndex];
  float trackedNote = 69.0f + (oscillatorNotes_[oscillatorIndex] - 69.0f) *
                                parameters.keyTracking;
  float finalNote = trackedNote + parameters.octave * 12.0f +
                    parameters.semitoneTune + parameters.fineTuneCents * 0.01f;
  frequencySmoothers_[oscillatorIndex].Snap(
    MidiToFrequency(Clamp(finalNote, kMinPitch, kMaxPitch))
  );
}

void Voice::SetOscillatorGate(size_t oscillatorIndex, bool enabled) {
  if (oscillatorIndex >= kOscillatorsPerVoice) return;
  oscillatorGates_[oscillatorIndex] = enabled;
}

void Voice::RefreshEnvelope(size_t envelopeIndex) {
  if (envelopeIndex >= kEnvelopesPerVoice) return;
  const EnvelopeParameters& parameters = patch_.envelopes[envelopeIndex];
  envelopes_[envelopeIndex].SetAttackTime(parameters.attackSeconds);
  envelopes_[envelopeIndex].SetDecayTime(parameters.decaySeconds);
  envelopes_[envelopeIndex].SetSustainLevel(parameters.sustain);
  envelopes_[envelopeIndex].SetReleaseTime(parameters.releaseSeconds);
}

void Voice::RefreshLfo(size_t lfoIndex) {
  if (lfoIndex >= kLfosPerVoice) return;
  lfos_[lfoIndex].SetParameters(patch_.lfos[lfoIndex]);
}

void Voice::UpdateControlRateParameters(
  const std::array<float, static_cast<size_t>(ModDestination::Count)>& modulation) {
  float allPitch = ModValue(modulation, ModDestination::AllOscPitch) * 24.0f;
  for (size_t oscillatorIndex = 0;
       oscillatorIndex < kOscillatorsPerVoice;
       ++oscillatorIndex) {
    const OscillatorParameters& parameters = patch_.oscillators[oscillatorIndex];
    ModDestination pitchDestination = OscillatorDestination(
      oscillatorIndex, ModDestination::Osc1Pitch
    );
    ModDestination pulseWidthDestination = OscillatorDestination(
      oscillatorIndex, ModDestination::Osc1PulseWidth
    );
    ModDestination wavefoldDestination = OscillatorDestination(
      oscillatorIndex, ModDestination::Osc1Wavefold
    );

    float trackedNote = 69.0f + (oscillatorNotes_[oscillatorIndex] - 69.0f) *
                                  parameters.keyTracking;
    float pitchModulation = (allPitch + ModValue(modulation, pitchDestination) * 24.0f) *
                            parameters.modulationInputAmount;
    float finalNote = trackedNote + parameters.octave * 12.0f +
                      parameters.semitoneTune + parameters.fineTuneCents * 0.01f +
                      pitchModulation;
    frequencySmoothers_[oscillatorIndex].SetTarget(
      MidiToFrequency(Clamp(finalNote, kMinPitch, kMaxPitch))
    );

    float pulseWidth = parameters.pulseWidth +
                       ModValue(modulation, pulseWidthDestination) * 0.49f;
    oscillators_[oscillatorIndex].SetPw(Clamp(pulseWidth, 0.01f, 0.99f));

    float wavefold = parameters.wavefold + ModValue(modulation, wavefoldDestination);
    wavefolders_[oscillatorIndex].SetGain(1.0f + Clamp(wavefold, 0.0f, 1.0f) * 3.0f);
  }

  float cutoffOctaves = ModValue(modulation, ModDestination::FilterCutoff) * 5.0f;
  float cutoff = patch_.voice.filterCutoffHz * powf(2.0f, cutoffOctaves);
  cutoffSmoother_.SetTarget(Clamp(cutoff, kMinFilterCutoffHz, sampleRate_ / 3.0f));
  resonanceSmoother_.SetTarget(Clamp(
    patch_.voice.filterResonance + ModValue(modulation, ModDestination::FilterResonance),
    0.0f,
    1.0f
  ));
}

bool Voice::AnyEnvelopeRunning() const {
  for (size_t envelope = 0; envelope < kEnvelopesPerVoice; ++envelope) {
    if (patch_.envelopes[envelope].enabled && envelopes_[envelope].IsRunning()) return true;
  }
  return false;
}

StereoFrame Voice::Process(GlobalModulation& globalModulation) {
  std::array<float, static_cast<size_t>(ModSource::Count)> sources = {};
  float velocityNormalized = velocity_ / 127.0f;

  for (size_t envelope = 0; envelope < kEnvelopesPerVoice; ++envelope) {
    // A disabled envelope is still clocked with a closed gate so disabling and
    // later re-enabling it cannot resurrect stale ADSR state.
    float envelopeValue = envelopes_[envelope].Process(
      patch_.envelopes[envelope].enabled && gateState_
    );
    float value = patch_.envelopes[envelope].enabled ? envelopeValue : 0.0f;
    float velocityScale = 1.0f +
                          (velocityNormalized - 1.0f) *
                            patch_.envelopes[envelope].velocitySensitivity;
    sources[SourceIndex(static_cast<ModSource>(
      static_cast<uint8_t>(ModSource::Env1) + static_cast<uint8_t>(envelope)
    ))] = value * patch_.envelopes[envelope].amount * velocityScale;
  }
  for (size_t lfo = 0; lfo < kLfosPerVoice; ++lfo) {
    sources[SourceIndex(static_cast<ModSource>(
      static_cast<uint8_t>(ModSource::Lfo1) + static_cast<uint8_t>(lfo)
    ))] = lfos_[lfo].Process(active_);
  }
  sources[SourceIndex(ModSource::Velocity)] = velocityNormalized;
  sources[SourceIndex(ModSource::NoteNumber)] = midiNote_ / 127.0f;
  sources[SourceIndex(ModSource::KeyTrack)] = Clamp((midiNote_ - 60.0f) / 60.0f, -1.0f, 1.0f);

  std::array<float, static_cast<size_t>(ModDestination::Count)> modulation = {};
  modulationMatrix_.Evaluate(sources, modulation);
  if (!active_) return StereoFrame{};

  globalModulation.delayMix += ModValue(modulation, ModDestination::DelayMix);
  globalModulation.reverbMix += ModValue(modulation, ModDestination::ReverbMix);

  if (++controlRateCounter_ >= kControlRatePeriod) {
    controlRateCounter_ = 0;
    UpdateControlRateParameters(modulation);
  }

  std::array<float, kOscillatorsPerVoice> oscillatorSamples = {};
  std::array<float, kOscillatorsPerVoice> oscillatorLevels = {};
  float panWeighted = patch_.voice.pan;
  float panWeight = 1.0f;
  for (size_t oscillator = 0; oscillator < kOscillatorsPerVoice; ++oscillator) {
    ModDestination levelDestination = OscillatorDestination(
      oscillator, ModDestination::Osc1Level
    );
    float level = patch_.oscillators[oscillator].level +
                  ModValue(modulation, levelDestination);
    levelSmoothers_[oscillator].SetTarget(Clamp(level, 0.0f, 1.0f));
    oscillatorLevels[oscillator] = levelSmoothers_[oscillator].Process();

    oscillators_[oscillator].SetFreq(frequencySmoothers_[oscillator].Process());
    bool audible = patch_.oscillators[oscillator].enabled && oscillatorGates_[oscillator];
    if (audible && oscillatorLevels[oscillator] > 0.00001f) {
      oscillatorSamples[oscillator] = wavefolders_[oscillator].Process(
        oscillators_[oscillator].Process()
      );
      panWeighted += patch_.oscillators[oscillator].pan * oscillatorLevels[oscillator];
      panWeight += oscillatorLevels[oscillator];
    } else {
      oscillatorLevels[oscillator] = 0.0f;
    }
  }

  float noiseLevel = patch_.voice.noiseLevel + ModValue(modulation, ModDestination::NoiseLevel);
  noiseLevelSmoother_.SetTarget(Clamp(noiseLevel, 0.0f, 1.0f));
  noiseLevel = noiseLevelSmoother_.Process();
  float noiseSample = noiseLevel > 0.00001f ? noise_.Process() : 0.0f;
  float mixed = mixer_.Process(oscillatorSamples, oscillatorLevels, noiseSample, noiseLevel);

  if (patch_.voice.filterEnabled) {
    filter_.SetFreq(cutoffSmoother_.Process());
    filter_.SetRes(resonanceSmoother_.Process());
    filter_.Process(mixed);
    switch (patch_.voice.filterMode) {
      case FilterMode::HighPass: mixed = filter_.High(); break;
      case FilterMode::BandPass: mixed = filter_.Band(); break;
      case FilterMode::LowPass:
      default: mixed = filter_.Low(); break;
    }
  } else {
    cutoffSmoother_.Process();
    resonanceSmoother_.Process();
  }

  float amp = patch_.voice.amp + ModValue(modulation, ModDestination::Amp);
  vca_.SetGain(Clamp(amp, 0.0f, 1.0f));
  float output = vca_.Process(mixed);

  float pan = panWeighted / panWeight + ModValue(modulation, ModDestination::Pan);
  panSmoother_.SetTarget(Clamp(pan, -1.0f, 1.0f));
  pan = panSmoother_.Process();
  float leftGain = pan > 0.0f ? 1.0f - pan : 1.0f;
  float rightGain = pan < 0.0f ? 1.0f + pan : 1.0f;

  if (!gateState_ && !AnyEnvelopeRunning()) {
    active_ = false;
    velocity_ = 0;
    oscillatorGates_.fill(false);
  }

  StereoFrame frame;
  frame.left = output * leftGain;
  frame.right = output * rightGain;
  return frame;
}

float Voice::MidiToFrequency(float midiNote) {
  return Clamp(440.0f * powf(2.0f, (midiNote - 69.0f) / 12.0f),
               kMinFrequencyHz,
               22000.0f);
}

void SynthEngine::Init(float sampleRate) {
  allocationCounter_ = 0;
  reservedVoiceCount_ = 0;
  globalModulation_ = {};
  for (size_t voice = 0; voice < kMaxVoices; ++voice) {
    voices_[voice].Init(sampleRate, static_cast<uint32_t>(voice * 7919U + 1U));
  }
}

size_t SynthEngine::AllocateVoice() const {
  size_t firstCandidate = reservedVoiceCount_;
  for (size_t voice = firstCandidate; voice < kMaxVoices; ++voice) {
    if (!voices_[voice].IsActive()) return voice;
  }

  size_t oldestVoice = firstCandidate;
  for (size_t voice = firstCandidate + 1; voice < kMaxVoices; ++voice) {
    if (voices_[voice].AllocationAge() < voices_[oldestVoice].AllocationAge()) {
      oldestVoice = voice;
    }
  }
  return oldestVoice;
}

void SynthEngine::NoteOn(uint8_t note, uint8_t velocity) {
  size_t voice = AllocateVoice();
  voices_[voice].NoteOn(note, velocity, ++allocationCounter_);
}

void SynthEngine::NoteOff(uint8_t note) {
  for (size_t voice = reservedVoiceCount_; voice < kMaxVoices; ++voice) {
    if (voices_[voice].IsActive() && voices_[voice].MidiNote() == note) {
      voices_[voice].NoteOff();
    }
  }
}

void SynthEngine::AllNotesOff() {
  reservedVoiceCount_ = 0;
  for (size_t voice = 0; voice < kMaxVoices; ++voice) {
    voices_[voice].AllNotesOff();
  }
}

void SynthEngine::Process(float& left, float& right) {
  left = 0.0f;
  right = 0.0f;
  globalModulation_ = {};
  size_t activeVoices = 0;

  for (size_t voice = 0; voice < kMaxVoices; ++voice) {
    bool wasActive = voices_[voice].IsActive();
    StereoFrame frame = voices_[voice].Process(globalModulation_);
    if (wasActive) {
      left += frame.left;
      right += frame.right;
      activeVoices++;
    }
  }

  if (activeVoices > 1) {
    float normalization = 1.0f / static_cast<float>(activeVoices);
    left *= normalization;
    right *= normalization;
    globalModulation_.delayMix *= normalization;
    globalModulation_.reverbMix *= normalization;
  }
  globalModulation_.delayMix = Clamp(globalModulation_.delayMix, -1.0f, 1.0f);
  globalModulation_.reverbMix = Clamp(globalModulation_.reverbMix, -1.0f, 1.0f);
}

void SynthEngine::SetVoiceParameter(size_t voiceIndex,
                                    VoiceParameter parameter,
                                    float value) {
  if (voiceIndex < kMaxVoices) voices_[voiceIndex].SetVoiceParameter(parameter, value);
}

void SynthEngine::SetOscillatorParameter(size_t voiceIndex,
                                         size_t oscillatorIndex,
                                         OscillatorParameter parameter,
                                         float value) {
  if (voiceIndex < kMaxVoices) {
    voices_[voiceIndex].SetOscillatorParameter(oscillatorIndex, parameter, value);
  }
}

void SynthEngine::SetEnvelopeParameter(size_t voiceIndex,
                                       size_t envelopeIndex,
                                       EnvelopeParameter parameter,
                                       float value) {
  if (voiceIndex < kMaxVoices) {
    voices_[voiceIndex].SetEnvelopeParameter(envelopeIndex, parameter, value);
  }
}

void SynthEngine::SetLfoParameter(size_t voiceIndex,
                                  size_t lfoIndex,
                                  LfoParameter parameter,
                                  float value) {
  if (voiceIndex < kMaxVoices) {
    voices_[voiceIndex].SetLfoParameter(lfoIndex, parameter, value);
  }
}

void SynthEngine::SetModulationRoute(size_t voiceIndex,
                                     size_t routeIndex,
                                     const ModulationRoute& route) {
  if (voiceIndex < kMaxVoices) voices_[voiceIndex].SetModulationRoute(routeIndex, route);
}

void SynthEngine::ApplyVoicePatch(size_t voiceIndex, const VoicePatch& patch) {
  if (voiceIndex < kMaxVoices) voices_[voiceIndex].ApplyPatch(patch);
}

void SynthEngine::ReserveSequencerVoices(bool reserved) {
  reservedVoiceCount_ = reserved ? kSequencerVoices : 0;
  if (!reserved) {
    for (size_t voice = 0; voice < kSequencerVoices; ++voice) {
      voices_[voice].NoteOff();
    }
  }
}

void SynthEngine::TriggerSequencerVoice(size_t track, uint8_t note, uint8_t velocity) {
  if (track >= kSequencerVoices) return;
  reservedVoiceCount_ = kSequencerVoices;
  voices_[track].NoteOn(note, velocity, ++allocationCounter_);
}

void SynthEngine::SetSequencerGate(size_t track, bool gate) {
  if (track < kSequencerVoices) voices_[track].SetGate(gate);
}

void SynthEngine::SetSequencerOscillatorGate(size_t track, size_t oscillator, bool enabled) {
  if (track < kSequencerVoices) voices_[track].SetOscillatorGate(oscillator, enabled);
}

void SynthEngine::ReserveCompatibilityVoice(bool reserved) {
  reservedVoiceCount_ = reserved ? 1 : 0;
  if (!reserved) voices_[0].NoteOff();
}

void SynthEngine::SetCompatibilityOscillatorNote(size_t oscillatorIndex, float midiNote) {
  voices_[0].SetOscillatorNote(oscillatorIndex, midiNote);
}

void SynthEngine::SetCompatibilityOscillatorGate(size_t oscillatorIndex, bool enabled) {
  voices_[0].SetOscillatorGate(oscillatorIndex, enabled);
}

void SynthEngine::TriggerCompatibilityVoice(uint8_t referenceNote,
                                            uint8_t velocity,
                                            bool hardRetrigger) {
  reservedVoiceCount_ = 1;
  voices_[0].NoteOn(referenceNote, velocity, ++allocationCounter_);
  if (hardRetrigger) voices_[0].Retrigger(true);
}

void SynthEngine::SetCompatibilityGate(bool gate) {
  voices_[0].SetGate(gate);
}

size_t SynthEngine::ActiveVoiceCount() const {
  size_t count = 0;
  for (size_t voice = 0; voice < kMaxVoices; ++voice) {
    if (voices_[voice].IsActive()) count++;
  }
  return count;
}

const Voice& SynthEngine::GetVoice(size_t voiceIndex) const {
  return voices_[voiceIndex < kMaxVoices ? voiceIndex : 0];
}

}  // namespace PolyDsp
