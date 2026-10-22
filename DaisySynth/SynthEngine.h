#pragma once

#include "daisysp.h"

#include <array>
#include <stddef.h>
#include <stdint.h>

namespace PolyDsp {

constexpr size_t kMaxVoices = 4;
constexpr size_t kSequencerVoices = 2;
constexpr size_t kOscillatorsPerVoice = 3;
constexpr size_t kEnvelopesPerVoice = 3;
constexpr size_t kLfosPerVoice = 2;
constexpr size_t kMaxModRoutesPerVoice = 16;

static_assert(kMaxVoices == 4, "The Daisy engine must expose four voices");
static_assert(kSequencerVoices < kMaxVoices, "Keep voices available for MIDI allocation");
static_assert(kOscillatorsPerVoice == 3, "Each voice must expose three oscillators");
static_assert(kEnvelopesPerVoice == 3, "Each voice must expose three envelopes");
static_assert(kLfosPerVoice == 2, "Each voice must expose two LFOs");

enum class FilterMode : uint8_t {
  LowPass,
  HighPass,
  BandPass,
};

enum class LfoWaveform : uint8_t {
  Sine,
  Triangle,
  Saw,
  Ramp,
  Square,
  SampleAndHold,
};

enum class RetriggerMode : uint8_t {
  Legato,
  Soft,
  Hard,
};

enum class ModSource : uint8_t {
  Env1,
  Env2,
  Env3,
  Lfo1,
  Lfo2,
  Velocity,
  NoteNumber,
  KeyTrack,
  Count,
};

enum class ModDestination : uint8_t {
  Amp,
  FilterCutoff,
  FilterResonance,

  Osc1Pitch,
  Osc2Pitch,
  Osc3Pitch,
  AllOscPitch,

  Osc1Level,
  Osc2Level,
  Osc3Level,
  NoiseLevel,

  Osc1PulseWidth,
  Osc2PulseWidth,
  Osc3PulseWidth,

  Osc1Wavefold,
  Osc2Wavefold,
  Osc3Wavefold,

  Pan,
  DelayMix,
  ReverbMix,
  Count,
};

enum class ModCurve : uint8_t {
  Linear,
  // Reserved for patch compatibility with future exponential/bipolar curves.
  Reserved,
};

enum class ModulationScope : uint8_t {
  PerVoice,
  Global,
};

struct ModulationRoute {
  bool enabled = false;
  ModSource source = ModSource::Env1;
  ModDestination destination = ModDestination::Amp;
  float amount = 0.0f;
  ModCurve curve = ModCurve::Linear;
  ModulationScope scope = ModulationScope::PerVoice;
};

struct OscillatorParameters {
  bool enabled = false;
  uint8_t waveform = 0;
  float level = 0.0f;
  int8_t octave = 0;
  int8_t semitoneTune = 0;
  float fineTuneCents = 0.0f;
  float phaseOffset = 0.0f;
  float pulseWidth = 0.5f;
  float wavefold = 0.0f;
  float pan = 0.0f;
  float keyTracking = 1.0f;
  float modulationInputAmount = 1.0f;
};

struct EnvelopeParameters {
  bool enabled = false;
  float attackSeconds = 0.01f;
  float decaySeconds = 0.08f;
  float sustain = 0.68f;
  float releaseSeconds = 0.32f;
  float amount = 1.0f;
  float velocitySensitivity = 0.0f;
  RetriggerMode retriggerMode = RetriggerMode::Soft;
};

struct LfoParameters {
  bool enabled = false;
  LfoWaveform waveform = LfoWaveform::Sine;
  float frequencyHz = 1.0f;
  float phase = 0.0f;
  bool bipolar = true;
  float amount = 0.0f;
  bool retriggerOnNoteOn = false;
  bool freeRunning = true;
  bool tempoSync = false;
  uint8_t tempoDivision = 0;
};

struct VoiceParameters {
  bool filterEnabled = false;
  FilterMode filterMode = FilterMode::LowPass;
  float filterCutoffHz = 2400.0f;
  float filterResonance = 0.15f;
  float filterDrive = 0.0f;
  float amp = 0.0f;
  float pan = 0.0f;
  float noiseLevel = 0.0f;
};

// Fixed-size runtime patch applied to every Daisy voice. UART v4 and ESP32
// preset v8 expose the same modern parameters; legacy presets are expanded on
// the ESP32 before their snapshots reach this structure.
struct VoicePatch {
  VoiceParameters voice;
  std::array<OscillatorParameters, kOscillatorsPerVoice> oscillators;
  std::array<EnvelopeParameters, kEnvelopesPerVoice> envelopes;
  std::array<LfoParameters, kLfosPerVoice> lfos;
  std::array<ModulationRoute, kMaxModRoutesPerVoice> routes;
};

struct StereoFrame {
  float left = 0.0f;
  float right = 0.0f;
};

struct GlobalModulation {
  float delayMix = 0.0f;
  float reverbMix = 0.0f;
};

enum class VoiceParameter : uint8_t {
  FilterEnabled,
  FilterMode,
  FilterCutoff,
  FilterResonance,
  FilterDrive,
  Amp,
  Pan,
  NoiseLevel,
};

enum class OscillatorParameter : uint8_t {
  Enabled,
  Waveform,
  Level,
  Octave,
  SemitoneTune,
  FineTune,
  PhaseOffset,
  PulseWidth,
  Wavefold,
  Pan,
  KeyTracking,
  ModulationInputAmount,
};

enum class EnvelopeParameter : uint8_t {
  Enabled,
  Attack,
  Decay,
  Sustain,
  Release,
  Amount,
  VelocitySensitivity,
  RetriggerMode,
};

enum class LfoParameter : uint8_t {
  Enabled,
  Waveform,
  Frequency,
  Phase,
  Bipolar,
  Amount,
  RetriggerOnNoteOn,
  FreeRunning,
  TempoSync,
  TempoDivision,
};

class SmoothedParameter {
 public:
  void Init(float initialValue, float coefficient);
  void SetTarget(float target);
  void Snap(float value);
  float Process();
  float Current() const { return current_; }

 private:
  float current_ = 0.0f;
  float target_ = 0.0f;
  float coefficient_ = 1.0f;
};

class Vca {
 public:
  void Init(float smoothingCoefficient);
  void SetGain(float gain);
  float Process(float input);

 private:
  SmoothedParameter gain_;
};

class RandomGenerator {
 public:
  void Seed(uint32_t seed);
  float NextBipolar();

 private:
  uint32_t state_ = 1;
};

class NoiseGenerator {
 public:
  void Init(uint32_t seed) { random_.Seed(seed); }
  float Process() { return random_.NextBipolar(); }

 private:
  RandomGenerator random_;
};

// One main oscillator with the complete 16-shape UI vocabulary. DaisySP's
// band-limited primitives render the basic/PWM shapes; the additional musical
// shapes share one fixed-size phase engine and never allocate.
class VoiceOscillator {
 public:
  void Init(float sampleRate, uint32_t seed);
  void SetWaveform(uint8_t waveform);
  void SetFreq(float frequencyHz);
  void SetPw(float pulseWidth);
  void Reset(float phase = 0.0f);
  float Process();

 private:
  static uint8_t MapDaisyWaveform(uint8_t waveform);
  static bool UsesDaisyWaveform(uint8_t waveform);
  static float WrapPhase(float phase);
  static float Sine(float phase);
  static float Saw(float phase);
  void AdvancePhases();
  float RenderExtendedWaveform();

  daisysp::Oscillator base_;
  RandomGenerator random_;
  float sampleRate_ = 48000.0f;
  float frequencyHz_ = 100.0f;
  float phase_ = 0.0f;
  float subPhase_ = 0.0f;
  float superPhaseLow_ = 0.0f;
  float superPhaseHigh_ = 0.0f;
  float pulseWidth_ = 0.5f;
  uint8_t waveform_ = 0;
};

class Lfo {
 public:
  void Init(float sampleRate, uint32_t seed);
  void SetParameters(const LfoParameters& parameters);
  void Retrigger();
  float Process(bool voiceActive);

 private:
  float RenderWaveform(float phase) const;

  LfoParameters parameters_;
  float sampleRate_ = 48000.0f;
  float phase_ = 0.0f;
  float heldValue_ = 0.0f;
  RandomGenerator random_;
};

class ModulationMatrix {
 public:
  void Reset();
  void SetRoute(size_t index, const ModulationRoute& route);
  const ModulationRoute& GetRoute(size_t index) const;
  void Evaluate(const std::array<float, static_cast<size_t>(ModSource::Count)>& sources,
                std::array<float, static_cast<size_t>(ModDestination::Count)>& destinations) const;

 private:
  std::array<ModulationRoute, kMaxModRoutesPerVoice> routes_;
};

class OscillatorMixer {
 public:
  float Process(const std::array<float, kOscillatorsPerVoice>& oscillatorSamples,
                const std::array<float, kOscillatorsPerVoice>& oscillatorLevels,
                float noiseSample,
                float noiseLevel) const;
};

class Voice {
 public:
  void Init(float sampleRate, uint32_t randomSeed);
  void ApplyPatch(const VoicePatch& patch);

  void NoteOn(uint8_t note, uint8_t velocity, uint32_t allocationAge);
  void NoteOff();
  void AllNotesOff(bool hard = false);
  void Retrigger(bool hard);
  void SetGate(bool gate);

  void SetVoiceParameter(VoiceParameter parameter, float value);
  void SetOscillatorParameter(size_t oscillatorIndex,
                              OscillatorParameter parameter,
                              float value);
  void SetEnvelopeParameter(size_t envelopeIndex,
                            EnvelopeParameter parameter,
                            float value);
  void SetLfoParameter(size_t lfoIndex, LfoParameter parameter, float value);
  void SetModulationRoute(size_t routeIndex, const ModulationRoute& route);

  void SetOscillatorNote(size_t oscillatorIndex, float midiNote);
  void SetOscillatorGate(size_t oscillatorIndex, bool enabled);

  StereoFrame Process(GlobalModulation& globalModulation);

  bool IsActive() const { return active_; }
  uint8_t MidiNote() const { return midiNote_; }
  uint8_t Velocity() const { return velocity_; }
  bool GateState() const { return gateState_; }
  uint32_t AllocationAge() const { return allocationAge_; }

 private:
  void ResetPatchToDefaults();
  void RefreshEnvelope(size_t envelopeIndex);
  void RefreshLfo(size_t lfoIndex);
  void UpdateControlRateParameters(
    const std::array<float, static_cast<size_t>(ModDestination::Count)>& modulation);
  bool AnyEnvelopeRunning() const;
  static float MidiToFrequency(float midiNote);

  std::array<VoiceOscillator, kOscillatorsPerVoice> oscillators_;
  std::array<daisysp::Wavefolder, kOscillatorsPerVoice> wavefolders_;
  NoiseGenerator noise_;
  OscillatorMixer mixer_;
  daisysp::Svf filter_;
  Vca vca_;
  std::array<daisysp::Adsr, kEnvelopesPerVoice> envelopes_;
  std::array<Lfo, kLfosPerVoice> lfos_;
  ModulationMatrix modulationMatrix_;

  VoicePatch patch_;
  std::array<float, kOscillatorsPerVoice> oscillatorNotes_;
  std::array<bool, kOscillatorsPerVoice> oscillatorGates_;
  std::array<SmoothedParameter, kOscillatorsPerVoice> frequencySmoothers_;
  std::array<SmoothedParameter, kOscillatorsPerVoice> levelSmoothers_;
  SmoothedParameter noiseLevelSmoother_;
  SmoothedParameter panSmoother_;
  SmoothedParameter cutoffSmoother_;
  SmoothedParameter resonanceSmoother_;

  float sampleRate_ = 48000.0f;
  bool active_ = false;
  uint8_t midiNote_ = 60;
  uint8_t velocity_ = 0;
  bool gateState_ = false;
  uint32_t allocationAge_ = 0;
  uint8_t controlRateCounter_ = 0;
};

class SynthEngine {
 public:
  void Init(float sampleRate);
  void NoteOn(uint8_t note, uint8_t velocity);
  void NoteOff(uint8_t note);
  void AllNotesOff();
  void Process(float& left, float& right);

  void SetVoiceParameter(size_t voiceIndex, VoiceParameter parameter, float value);
  void SetOscillatorParameter(size_t voiceIndex,
                              size_t oscillatorIndex,
                              OscillatorParameter parameter,
                              float value);
  void SetEnvelopeParameter(size_t voiceIndex,
                            size_t envelopeIndex,
                            EnvelopeParameter parameter,
                            float value);
  void SetLfoParameter(size_t voiceIndex,
                       size_t lfoIndex,
                       LfoParameter parameter,
                       float value);
  void SetModulationRoute(size_t voiceIndex,
                          size_t routeIndex,
                          const ModulationRoute& route);
  void ApplyVoicePatch(size_t voiceIndex, const VoicePatch& patch);

  // Each UART track owns a complete DSP voice and its three oscillators.
  void ReserveSequencerVoices(bool reserved);
  void TriggerSequencerVoice(size_t track, uint8_t note, uint8_t velocity);
  void SetSequencerGate(size_t track, bool gate);
  void SetSequencerOscillatorGate(size_t track, size_t oscillator, bool enabled);

  // Legacy single-voice API retained for existing engine clients/tests.
  void ReserveCompatibilityVoice(bool reserved);
  void SetCompatibilityOscillatorNote(size_t oscillatorIndex, float midiNote);
  void SetCompatibilityOscillatorGate(size_t oscillatorIndex, bool enabled);
  void TriggerCompatibilityVoice(uint8_t referenceNote,
                                 uint8_t velocity,
                                 bool hardRetrigger);
  void SetCompatibilityGate(bool gate);

  const GlobalModulation& GetGlobalModulation() const { return globalModulation_; }
  size_t ActiveVoiceCount() const;
  const Voice& GetVoice(size_t voiceIndex) const;

 private:
  size_t AllocateVoice() const;

  std::array<Voice, kMaxVoices> voices_;
  uint32_t allocationCounter_ = 0;
  size_t reservedVoiceCount_ = 0;
  GlobalModulation globalModulation_;
};

VoicePatch MakeDefaultVoicePatch();
bool IsGlobalDestination(ModDestination destination);

}  // namespace PolyDsp
