#include "daisy_seed.h"
#include "daisysp.h"
#include "SynthEngine.h"

#include <math.h>
#include <string.h>

#include "../TheSynthProject/DaisyLinkHealth.h"

#ifndef SYNTH_ENABLE_STARTUP_VALIDATION
#define SYNTH_ENABLE_STARTUP_VALIDATION 1
#endif
#ifndef SYNTH_LINK_DIAGNOSTIC
#define SYNTH_LINK_DIAGNOSTIC 0
#endif

using namespace daisy;
using namespace daisysp;
using namespace DaisyLinkProtocol;

namespace {

constexpr size_t AUDIO_BLOCK_SIZE = 48;
constexpr size_t UART_DMA_BUFFER_SIZE = 512;
constexpr uint32_t LINK_TIMEOUT_MS = 500;
// A Daisy powered before the ESP32 can occasionally leave USART1's circular
// DMA listener armed before the other side has finished booting.  Rearm only
// the UART if no valid control frame arrives; this is the software equivalent
// of the RESET workaround, without restarting the audio engine.
constexpr uint32_t UART_BOOTSTRAP_REARM_MS = 1500;
// The ESP32 playhead follows this audio-authoritative status. Twenty
// milliseconds is fast enough for the LEDs and leaves headroom while the
// polyphonic DSP is active.
constexpr uint32_t STATUS_PERIOD_MS = 20;
// A status frame needs about 3 ms on the wire at 115200 baud. The former 5 ms
// timeout could expire when audio interrupts preempted the polling transmit,
// leaving a partial frame that the ESP32 correctly rejected.
constexpr uint32_t STATUS_TX_TIMEOUT_MS = 25;
constexpr float OUTPUT_GAIN = 0.30f;
// A 14 ms gain transition is still audible as a pop on powered speakers.
// This gives an approximately 200 ms fade at 48 kHz for every logical
// enable/disable and after the first packet received at boot.
constexpr float AUDIO_GAIN_RAMP = 0.00025f;
constexpr size_t MAX_DELAY_SAMPLES = 48000U * 6U / 10U;
#if SYNTH_ENABLE_STARTUP_VALIDATION
constexpr size_t DSP_VALIDATION_FRAMES = 1024;
#endif

DaisySeed hardware;
UartHandler controlUart;

Oscillator testOscillator;
PolyDsp::SynthEngine synthEngine;
SyntheticBassDrum kick;
Overdrive kickDrive;
DelayLine<float, MAX_DELAY_SAMPLES> DSY_SDRAM_BSS delayLine;
ReverbSc reverb;

uint8_t DMA_BUFFER_MEM_SECTION uartDmaBuffer[UART_DMA_BUFFER_SIZE];

ControlPayload controlSlots[2] = {};
uint16_t controlSlotSequences[2] = {};
volatile uint8_t publishedControlSlot = 0;
volatile bool linkActive = false;
volatile uint32_t lastPacketMs = 0;
volatile uint32_t acceptedPackets = 0;
volatile uint32_t rejectedPackets = 0;
volatile uint32_t uartRxRestartCount = 0;
volatile uint32_t uartRxErrorCount = 0;
volatile bool uartRxRestartRequested = false;

ControlPayload activeControl = {};
uint16_t activeControlSequence = 0xFFFFU;
uint16_t statusSequence = 0;
volatile uint8_t currentStep = 0;
uint32_t stepSample = 0;
uint32_t samplesPerStep = 6000;
uint32_t gateSamples = 4200;
float sampleRate = 48000.0f;
float masterGain = 0.0f;
float linkGain = 0.0f;
float delaySamples = 12000.0f;
bool kickTriggerPending = false;
bool synthPatchConfigured = false;

#if SYNTH_ENABLE_STARTUP_VALIDATION
// Debugger-visible results from the deterministic boot validation. The test
// runs before StartAudio(), never from the realtime callback.
volatile bool dspStartupValidationPassed = false;
volatile uint32_t dspWorstCaseElapsedUs = 0;
volatile float dspWorstCaseLoadPercent = 0.0f;
volatile float dspValidationPeak = 0.0f;
#endif

float clampFloat(float value, float minimum, float maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}

uint8_t clampByte(uint8_t value, uint8_t maximum) {
  return value > maximum ? maximum : value;
}

bool hasControlFlag(uint8_t flag) {
  return (activeControl.flags & flag) != 0U;
}

bool isStepEnabled(uint64_t mask, uint8_t step) {
  return (mask & (uint64_t)(1ULL << step)) != 0U;
}

void publishControl(const ControlPacket& packet) {
  uint8_t nextSlot = (uint8_t)(publishedControlSlot ^ 1U);
  memcpy(&controlSlots[nextSlot], &packet.payload, sizeof(ControlPayload));
  controlSlotSequences[nextSlot] = packet.header.sequence;
  __DMB();
  publishedControlSlot = nextSlot;
  lastPacketMs = System::GetNow();
  linkActive = true;
  acceptedPackets++;
}

// UART frames are already delimited by their header. After an invalid control
// snapshot, discard it completely: scanning its arbitrary payload for another
// header can lock the receiver onto a coincidental D5/59 byte pair.
class ControlPacketParser {
 public:
  void Reset() { index_ = 0; }

  void Push(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) PushByte(data[i]);
  }

 private:
  void PushByte(uint8_t byte) {
    if (index_ == 0 && byte != SYNC_0) return;
    if (index_ == 1 && byte != SYNC_1) {
      index_ = byte == SYNC_0 ? 1U : 0U;
      buffer_[0] = SYNC_0;
      return;
    }
    buffer_[index_++] = byte;
    if (index_ != sizeof(ControlPacket)) return;

    ControlPacket packet;
    memcpy(&packet, buffer_, sizeof(packet));
    if (isPacketValid<ControlPacket, ControlPayload>(packet, CONTROL_PACKET_TYPE)) {
      publishControl(packet);
    } else {
      ++rejectedPackets;
    }
    index_ = 0;
  }

  uint8_t buffer_[sizeof(ControlPacket)] = {};
  size_t index_ = 0;
};

ControlPacketParser controlParser;

void uartReceiveCallback(uint8_t* data,
                         size_t size,
                         void*,
                         UartHandler::Result result) {
  if (result == UartHandler::Result::OK) {
    controlParser.Push(data, size);
  } else {
    rejectedPackets++;
    uartRxErrorCount++;
    linkActive = false;
    uartRxRestartRequested = true;
  }
}

UartHandler::Config makeUartConfig() {
  UartHandler::Config config;
  config.periph = UartHandler::Config::Peripheral::USART_1;
  config.mode = UartHandler::Config::Mode::TX_RX;
  config.pin_config.tx = seed::D13;  // Pin fisico 14 -> ESP32 RX2.
  config.pin_config.rx = seed::D14;  // Pin fisico 15 <- ESP32 TX2.
  config.baudrate = UART_BAUD_RATE;
  return config;
}

bool startUartReceiver(bool recovery) {
  if (recovery) {
    // libDaisy's circular listener is not rearmed after every UART error.
    // Stop and reinitialize the peripheral so an ESP32 power cycle cannot
    // leave the Daisy transmit-only until its next manual reset.
    controlUart.DmaListenStop();
  }

  controlParser.Reset();
  UartHandler::Config config = makeUartConfig();
  if (controlUart.Init(config) != UartHandler::Result::OK) {
    uartRxErrorCount++;
    return false;
  }

  UartHandler::Result result = controlUart.DmaListenStart(
    uartDmaBuffer,
    UART_DMA_BUFFER_SIZE,
    uartReceiveCallback,
    nullptr
  );
  if (result != UartHandler::Result::OK) {
    uartRxErrorCount++;
    return false;
  }

  if (recovery) uartRxRestartCount++;
  return true;
}

bool anyVoiceEnabledAtStep(uint8_t step) {
  step = step < STEP_COUNT ? step : 0;
  for (uint8_t voice = 0; voice < VOICE_COUNT; ++voice) {
    if (isStepEnabled(activeControl.voiceEnabledMask[voice], step)) {
      return true;
    }
  }
  return false;
}

bool anyVoiceEnabledAtCurrentStep() {
  return anyVoiceEnabledAtStep(currentStep);
}

bool envelopeGateIsOpen();

bool synthPatchChanged(const ControlPayload& previous,
                       const ControlPayload& current) {
  constexpr uint8_t kFilterFlag = CONTROL_FILTER_ENABLED;
  if (((previous.flags ^ current.flags) & kFilterFlag) != 0U) return true;
  if (previous.filterType != current.filterType ||
      previous.filterCutoffHz != current.filterCutoffHz ||
      previous.filterResonancePercent != current.filterResonancePercent ||
      previous.noiseLevelPercent != current.noiseLevelPercent ||
      previous.voicePanPercent != current.voicePanPercent) {
    return true;
  }

  return memcmp(previous.oscillators,
                current.oscillators,
                sizeof(current.oscillators)) != 0 ||
         memcmp(previous.envelopes,
                current.envelopes,
                sizeof(current.envelopes)) != 0 ||
         memcmp(previous.lfos,
                current.lfos,
                sizeof(current.lfos)) != 0 ||
         memcmp(previous.modulationRoutes,
                current.modulationRoutes,
                sizeof(current.modulationRoutes)) != 0;
}

void configureSynthEngineFromControl() {
  for (size_t engineVoice = 0; engineVoice < PolyDsp::kMaxVoices; ++engineVoice) {
    for (size_t oscillator = 0;
         oscillator < PolyDsp::kOscillatorsPerVoice;
         ++oscillator) {
      const OscillatorControlParameters& parameters =
        activeControl.oscillators[oscillator];
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::Enabled,
        (parameters.flags & 1U) != 0U ? 1.0f : 0.0f
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::Waveform,
        parameters.waveform
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::Level,
        parameters.levelPercent / 100.0f
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::Octave,
        parameters.octave
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::SemitoneTune,
        parameters.semitoneTune
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::FineTune,
        parameters.fineTuneCents
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::PhaseOffset,
        parameters.phasePercent / 100.0f
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::PulseWidth,
        parameters.pulseWidthPercent / 100.0f
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::Wavefold,
        parameters.wavefoldPercent / 100.0f
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::Pan,
        parameters.panPercent / 100.0f
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::KeyTracking,
        parameters.keyTrackingPercent / 100.0f
      );
      synthEngine.SetOscillatorParameter(
        engineVoice,
        oscillator,
        PolyDsp::OscillatorParameter::ModulationInputAmount,
        parameters.modulationAmountPercent / 100.0f
      );
    }

    for (size_t envelope = 0;
         envelope < PolyDsp::kEnvelopesPerVoice;
         ++envelope) {
      const EnvelopeControlParameters& parameters = activeControl.envelopes[envelope];
      synthEngine.SetEnvelopeParameter(
        engineVoice,
        envelope,
        PolyDsp::EnvelopeParameter::Enabled,
        (parameters.flags & 1U) != 0U ? 1.0f : 0.0f
      );
      synthEngine.SetEnvelopeParameter(
        engineVoice, envelope, PolyDsp::EnvelopeParameter::Attack,
        parameters.attackMs / 1000.0f
      );
      synthEngine.SetEnvelopeParameter(
        engineVoice, envelope, PolyDsp::EnvelopeParameter::Decay,
        parameters.decayMs / 1000.0f
      );
      synthEngine.SetEnvelopeParameter(
        engineVoice, envelope, PolyDsp::EnvelopeParameter::Sustain,
        parameters.sustainPercent / 100.0f
      );
      synthEngine.SetEnvelopeParameter(
        engineVoice, envelope, PolyDsp::EnvelopeParameter::Release,
        parameters.releaseMs / 1000.0f
      );
      synthEngine.SetEnvelopeParameter(
        engineVoice, envelope, PolyDsp::EnvelopeParameter::Amount,
        parameters.amountPercent / 100.0f
      );
      synthEngine.SetEnvelopeParameter(
        engineVoice, envelope, PolyDsp::EnvelopeParameter::VelocitySensitivity,
        parameters.velocitySensitivityPercent / 100.0f
      );
      synthEngine.SetEnvelopeParameter(
        engineVoice, envelope, PolyDsp::EnvelopeParameter::RetriggerMode,
        (parameters.flags >> 1) & 0x03U
      );
    }

    for (size_t lfo = 0; lfo < PolyDsp::kLfosPerVoice; ++lfo) {
      const DaisyLinkProtocol::LfoControlParameters& parameters =
        activeControl.lfos[lfo];
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::Enabled,
        (parameters.flags & 1U) != 0U ? 1.0f : 0.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::Waveform, parameters.waveform
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::Frequency,
        parameters.frequencyCentiHz / 100.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::Phase,
        parameters.phasePercent / 100.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::Bipolar,
        (parameters.flags & (1U << 1)) != 0U ? 1.0f : 0.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::Amount,
        parameters.amountPercent / 100.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::RetriggerOnNoteOn,
        (parameters.flags & (1U << 2)) != 0U ? 1.0f : 0.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::FreeRunning,
        (parameters.flags & (1U << 3)) != 0U ? 1.0f : 0.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::TempoSync,
        (parameters.flags & (1U << 4)) != 0U ? 1.0f : 0.0f
      );
      synthEngine.SetLfoParameter(
        engineVoice, lfo, PolyDsp::LfoParameter::TempoDivision,
        parameters.tempoDivision
      );
    }

    for (size_t route = 0; route < PolyDsp::kMaxModRoutesPerVoice; ++route) {
      const ModulationRouteParameters& packed = activeControl.modulationRoutes[route];
      PolyDsp::ModulationRoute parameters;
      parameters.enabled = (packed.flags & 1U) != 0U &&
                           packed.source < static_cast<uint8_t>(PolyDsp::ModSource::Count) &&
                           packed.destination < static_cast<uint8_t>(PolyDsp::ModDestination::Count);
      parameters.source = static_cast<PolyDsp::ModSource>(packed.source);
      parameters.destination = static_cast<PolyDsp::ModDestination>(packed.destination);
      parameters.amount = packed.amountPercent / 100.0f;
      synthEngine.SetModulationRoute(engineVoice, route, parameters);
    }

    synthEngine.SetVoiceParameter(
      engineVoice,
      PolyDsp::VoiceParameter::FilterEnabled,
      hasControlFlag(CONTROL_FILTER_ENABLED) ? 1.0f : 0.0f
    );
    synthEngine.SetVoiceParameter(
      engineVoice,
      PolyDsp::VoiceParameter::FilterMode,
      activeControl.filterType
    );
    synthEngine.SetVoiceParameter(
      engineVoice,
      PolyDsp::VoiceParameter::FilterCutoff,
      clampFloat(activeControl.filterCutoffHz, 40.0f, sampleRate / 3.0f)
    );
    synthEngine.SetVoiceParameter(
      engineVoice,
      PolyDsp::VoiceParameter::FilterResonance,
      activeControl.filterResonancePercent / 100.0f
    );
    synthEngine.SetVoiceParameter(
      engineVoice, PolyDsp::VoiceParameter::FilterDrive, 0.0f
    );
    synthEngine.SetVoiceParameter(
      engineVoice,
      PolyDsp::VoiceParameter::NoiseLevel,
      activeControl.noiseLevelPercent / 100.0f
    );
    synthEngine.SetVoiceParameter(
      engineVoice,
      PolyDsp::VoiceParameter::Pan,
      activeControl.voicePanPercent / 100.0f
    );
  }
}

uint8_t getCompatibilityReferenceNote(uint8_t step, bool preview);

void setCompatibilityOscillators(uint8_t step, bool preview) {
  step = step < STEP_COUNT ? step : 0;
  uint8_t previewVoice = activeControl.previewVoice < PolyDsp::kOscillatorsPerVoice
                           ? activeControl.previewVoice
                           : 0;
  uint8_t referenceNote = getCompatibilityReferenceNote(step, preview);
  bool melodicStepActive = anyVoiceEnabledAtStep(step);

  for (size_t oscillator = 0;
       oscillator < PolyDsp::kOscillatorsPerVoice;
       ++oscillator) {
    bool laneHasNote = oscillator < VOICE_COUNT &&
                       isStepEnabled(activeControl.voiceEnabledMask[oscillator], step);
    uint8_t note = laneHasNote
                     ? activeControl.stepMidiNote[oscillator][step]
                     : referenceNote;
    bool oscillatorEnabled = (activeControl.oscillators[oscillator].flags & 1U) != 0U;
    bool enabled = preview
                     ? oscillator == previewVoice
                     : oscillatorEnabled &&
                         hasControlFlag(CONTROL_SEQUENCER_ENABLED) &&
                         melodicStepActive;
    synthEngine.SetCompatibilityOscillatorNote(
      oscillator, note
    );
    synthEngine.SetCompatibilityOscillatorGate(oscillator, enabled);
  }
}

uint8_t getCompatibilityReferenceNote(uint8_t step, bool preview) {
  step = step < STEP_COUNT ? step : 0;
  if (preview) {
    uint8_t previewVoice = activeControl.previewVoice < VOICE_COUNT
                             ? activeControl.previewVoice
                             : 0;
    return activeControl.stepMidiNote[previewVoice][step];
  }
  for (size_t oscillator = 0; oscillator < VOICE_COUNT; ++oscillator) {
    if (isStepEnabled(activeControl.voiceEnabledMask[oscillator], step)) {
      return activeControl.stepMidiNote[oscillator][step];
    }
  }
  return activeControl.stepMidiNote[0][step];
}

void triggerCompatibilityVoice(uint8_t step, bool preview) {
  bool hasNotes = preview || anyVoiceEnabledAtStep(step);
  synthEngine.ReserveCompatibilityVoice(preview ||
                                        hasControlFlag(CONTROL_TRANSPORT_RUNNING));
  if (!hasNotes) {
    setCompatibilityOscillators(step, preview);
    synthEngine.SetCompatibilityGate(false);
    return;
  }

  synthEngine.TriggerCompatibilityVoice(
    getCompatibilityReferenceNote(step, preview), 127, false
  );
  // NoteOn initializes the general polyphonic oscillator gates. Apply the
  // legacy per-track notes and step masks afterwards.
  setCompatibilityOscillators(step, preview);
  synthEngine.SetCompatibilityGate(true);
}

void triggerCurrentStep() {
  if (!hasControlFlag(CONTROL_PREVIEW_ENABLED)) {
    triggerCompatibilityVoice(currentStep, false);
  }

  if (activeControl.kickLevel > 0 &&
      isStepEnabled(activeControl.kickEnabledMask, currentStep)) {
    kickTriggerPending = true;
  }
}

void updateClockSettings() {
  uint8_t bpm = activeControl.bpm;
  if (bpm < 40) bpm = 40;
  if (bpm > 240) bpm = 240;
  samplesPerStep = (uint32_t)(sampleRate * 15.0f / bpm);
  if (samplesPerStep == 0) samplesPerStep = 1;
  gateSamples = samplesPerStep * 70U / 100U;
  if (gateSamples == 0) gateSamples = 1;
}

void applyControlAtBlockBoundary() {
  if (!linkActive) return;

  uint8_t slot = publishedControlSlot;
  uint16_t sequence = controlSlotSequences[slot];
  if (sequence == activeControlSequence) return;

  ControlPayload previous = activeControl;
  memcpy(&activeControl, &controlSlots[slot], sizeof(activeControl));
  activeControlSequence = sequence;

  activeControl.sequenceLength = clampByte(activeControl.sequenceLength, STEP_COUNT);
  if (activeControl.sequenceLength == 0) activeControl.sequenceLength = 1;
  activeControl.selectedStep %= activeControl.sequenceLength;
  activeControl.previewVoice = clampByte(
    activeControl.previewVoice, PolyDsp::kOscillatorsPerVoice - 1
  );

  updateClockSettings();
  // Control snapshots arrive every 40 ms even when the patch is unchanged.
  // Reapplying hundreds of DSP parameters inside the audio callback can miss
  // its deadline precisely when a sequencer note also activates rendering.
  // Pattern, clock and transport fields do not require a patch rebuild.
  if (!synthPatchConfigured || synthPatchChanged(previous, activeControl)) {
    configureSynthEngineFromControl();
    synthPatchConfigured = true;
  }

  bool wasRunning = (previous.flags & CONTROL_TRANSPORT_RUNNING) != 0U;
  bool isRunning = hasControlFlag(CONTROL_TRANSPORT_RUNNING);
  if (isRunning && !wasRunning) {
    currentStep = activeControl.selectedStep;
    stepSample = 0;
    triggerCurrentStep();
  } else if (!isRunning) {
    currentStep = activeControl.selectedStep;
    stepSample = 0;
  } else if (currentStep >= activeControl.sequenceLength) {
    currentStep = 0;
    stepSample = 0;
    triggerCurrentStep();
  }

  kick.SetFreq(clampFloat(activeControl.kickPitchHz, 25.0f, 120.0f));
  float kickDecay = clampFloat((activeControl.kickDecayMs - 80.0f) / 1120.0f, 0.0f, 1.0f);
  // SyntheticBassDrum squares its decay input internally. Square-rooting the
  // UI value keeps the 80-1200 ms control perceptually useful over its range.
  kick.SetDecay(sqrtf(kickDecay));
  // DaisySP's Tone parameter controls both the transient level and brightness,
  // which is the closest match for the classic 909 Attack control.
  kick.SetTone(activeControl.kickAttack / 100.0f);
  kick.SetDirtiness(0.18f);
  kick.SetFmEnvelopeAmount(clampFloat(activeControl.kickSweepHz / 260.0f, 0.0f, 1.0f));
  kick.SetFmEnvelopeDecay(0.28f);
  kick.SetAccent(0.8f);
  kickDrive.SetDrive(activeControl.kickDrive / 100.0f);

  reverb.SetFeedback(0.72f + activeControl.reverbRoom * 0.0023f);
  reverb.SetLpFreq(12000.0f);

  bool previewWasEnabled = (previous.flags & CONTROL_PREVIEW_ENABLED) != 0U;
  bool previewEnabled = hasControlFlag(CONTROL_PREVIEW_ENABLED);
  bool previewChanged = previous.previewVoice != activeControl.previewVoice ||
                        previous.selectedStep != activeControl.selectedStep;
  if (previewEnabled && (!previewWasEnabled || previewChanged)) {
    triggerCompatibilityVoice(activeControl.selectedStep, true);
  } else if (previewEnabled) {
    synthEngine.ReserveCompatibilityVoice(true);
    setCompatibilityOscillators(activeControl.selectedStep, true);
    synthEngine.SetCompatibilityGate(true);
  } else if (isRunning) {
    synthEngine.ReserveCompatibilityVoice(true);
    setCompatibilityOscillators(currentStep, false);
    synthEngine.SetCompatibilityGate(envelopeGateIsOpen());
  } else {
    setCompatibilityOscillators(activeControl.selectedStep, false);
    synthEngine.SetCompatibilityGate(false);
    synthEngine.ReserveCompatibilityVoice(false);
  }

  if (activeControl.kickTriggerSequence != previous.kickTriggerSequence &&
      activeControl.kickLevel > 0) {
    kickTriggerPending = true;
  }
}

void advanceSequencer() {
  if (!hasControlFlag(CONTROL_OUTPUT_ENABLED) ||
      !hasControlFlag(CONTROL_TRANSPORT_RUNNING) ||
      !hasControlFlag(CONTROL_SEQUENCER_ENABLED)) {
    return;
  }

  stepSample++;
  if (stepSample < samplesPerStep) return;

  stepSample = 0;
  currentStep = (uint8_t)((currentStep + 1U) % activeControl.sequenceLength);
  triggerCurrentStep();
}

bool envelopeGateIsOpen() {
  if (!hasControlFlag(CONTROL_OUTPUT_ENABLED)) return false;
  if (hasControlFlag(CONTROL_PREVIEW_ENABLED)) return true;
  if (!hasControlFlag(CONTROL_TRANSPORT_RUNNING) ||
      !hasControlFlag(CONTROL_SEQUENCER_ENABLED) ||
      stepSample >= gateSamples) {
    return false;
  }
  return anyVoiceEnabledAtCurrentStep();
}

float processKickDrive(float input) {
  float mix = clampFloat(activeControl.kickDrive / 100.0f, 0.0f, 1.0f);
  if (mix <= 0.0f) return input;

  // DaisySP Overdrive is not unity gain at low settings and reaches zero
  // output at Drive=0. Keep a full dry path so this control changes timbre
  // instead of acting as a second, mandatory kick level control.
  float driven = kickDrive.Process(input);
  return input + (driven - input) * mix;
}

void processDelay(float inputLeft,
                  float inputRight,
                  float& outputLeft,
                  float& outputRight) {
  float target = activeControl.delayTimeMs * sampleRate / 1000.0f;
  target = clampFloat(target, 1.0f, MAX_DELAY_SAMPLES - 2.0f);
  delaySamples += (target - delaySamples) * 0.00035f;
  delayLine.SetDelay(delaySamples);

  float delayed = delayLine.Read();
  float feedback = clampFloat(activeControl.delayFeedback / 100.0f, 0.0f, 0.90f);
  float monoInput = (inputLeft + inputRight) * 0.5f;
  delayLine.Write(clampFloat(monoInput + delayed * feedback, -1.0f, 1.0f));

  float mix = activeControl.delayMix / 100.0f +
              synthEngine.GetGlobalModulation().delayMix;
  mix = clampFloat(mix, 0.0f, 1.0f);
  outputLeft = inputLeft + (delayed - inputLeft) * mix;
  outputRight = inputRight + (delayed - inputRight) * mix;
}

void processReverb(float inputLeft, float inputRight, float& left, float& right) {
  float wetLeft = 0.0f;
  float wetRight = 0.0f;
  reverb.Process(inputLeft, inputRight, &wetLeft, &wetRight);

  float mix = activeControl.reverbMix / 100.0f +
              synthEngine.GetGlobalModulation().reverbMix;
  mix = clampFloat(mix, 0.0f, 1.0f);
  left = inputLeft + (wetLeft - inputLeft) * mix;
  right = inputRight + (wetRight - inputRight) * mix;
}

void initializeGlobalProcessors() {
  kick.Init(sampleRate);
  kickDrive.Init();
  delaySamples = 12000.0f;
  delayLine.Init();
  delayLine.SetDelay(delaySamples);
  reverb.Init(sampleRate);
  reverb.SetFeedback(0.82f);
  reverb.SetLpFreq(12000.0f);
}

#if SYNTH_ENABLE_STARTUP_VALIDATION
void configureWorstCaseValidationPatch() {
  for (size_t voice = 0; voice < PolyDsp::kMaxVoices; ++voice) {
    synthEngine.SetVoiceParameter(
      voice, PolyDsp::VoiceParameter::FilterEnabled, 1.0f
    );
    synthEngine.SetVoiceParameter(
      voice, PolyDsp::VoiceParameter::FilterCutoff, 4200.0f
    );
    synthEngine.SetVoiceParameter(
      voice, PolyDsp::VoiceParameter::FilterResonance, 0.35f
    );
    synthEngine.SetVoiceParameter(
      voice, PolyDsp::VoiceParameter::NoiseLevel, 0.08f
    );

    for (size_t oscillator = 0;
         oscillator < PolyDsp::kOscillatorsPerVoice;
         ++oscillator) {
      synthEngine.SetOscillatorParameter(
        voice, oscillator, PolyDsp::OscillatorParameter::Enabled, 1.0f
      );
      synthEngine.SetOscillatorParameter(
        voice, oscillator, PolyDsp::OscillatorParameter::Level, 0.30f
      );
      synthEngine.SetOscillatorParameter(
        voice,
        oscillator,
        PolyDsp::OscillatorParameter::Waveform,
        static_cast<float>(oscillator)
      );
    }

    for (size_t envelope = 0;
         envelope < PolyDsp::kEnvelopesPerVoice;
         ++envelope) {
      synthEngine.SetEnvelopeParameter(
        voice, envelope, PolyDsp::EnvelopeParameter::Enabled, 1.0f
      );
      synthEngine.SetEnvelopeParameter(
        voice, envelope, PolyDsp::EnvelopeParameter::Attack, 0.0f
      );
      synthEngine.SetEnvelopeParameter(
        voice, envelope, PolyDsp::EnvelopeParameter::Decay, 1.0f
      );
      synthEngine.SetEnvelopeParameter(
        voice, envelope, PolyDsp::EnvelopeParameter::Sustain, 0.8f
      );
    }

    for (size_t lfo = 0; lfo < PolyDsp::kLfosPerVoice; ++lfo) {
      synthEngine.SetLfoParameter(
        voice, lfo, PolyDsp::LfoParameter::Enabled, 1.0f
      );
      synthEngine.SetLfoParameter(
        voice, lfo, PolyDsp::LfoParameter::Frequency, 5.0f + lfo * 2.0f
      );
      synthEngine.SetLfoParameter(
        voice, lfo, PolyDsp::LfoParameter::Amount, 0.35f
      );
    }

    PolyDsp::ModulationRoute route;
    route.enabled = true;
    route.source = PolyDsp::ModSource::Env2;
    route.destination = PolyDsp::ModDestination::FilterCutoff;
    route.amount = 0.35f;
    synthEngine.SetModulationRoute(voice, 1, route);

    route.source = PolyDsp::ModSource::Env3;
    route.destination = PolyDsp::ModDestination::Osc3Wavefold;
    route.amount = 0.5f;
    synthEngine.SetModulationRoute(voice, 2, route);

    route.source = PolyDsp::ModSource::Lfo1;
    route.destination = PolyDsp::ModDestination::AllOscPitch;
    route.amount = 0.08f;
    synthEngine.SetModulationRoute(voice, 3, route);

    route.source = PolyDsp::ModSource::Lfo2;
    route.destination = PolyDsp::ModDestination::Osc1PulseWidth;
    route.amount = 0.3f;
    synthEngine.SetModulationRoute(voice, 4, route);

    route.destination = PolyDsp::ModDestination::DelayMix;
    route.amount = 0.15f;
    synthEngine.SetModulationRoute(voice, 5, route);

    route.destination = PolyDsp::ModDestination::ReverbMix;
    route.amount = 0.15f;
    synthEngine.SetModulationRoute(voice, 6, route);
  }
}

void runDspStartupValidation() {
  configureWorstCaseValidationPatch();
  synthEngine.NoteOn(60, 127);
  synthEngine.NoteOn(64, 112);
  synthEngine.NoteOn(67, 104);
  synthEngine.NoteOn(71, 96);
  bool fourVoicesAllocated = synthEngine.ActiveVoiceCount() == PolyDsp::kMaxVoices;

  // With all four voices occupied, the fifth note must replace voice 0: it is
  // the oldest voice because allocation starts at the first inactive slot.
  synthEngine.NoteOn(76, 120);
  bool oldestVoiceStolen =
    synthEngine.ActiveVoiceCount() == PolyDsp::kMaxVoices &&
    synthEngine.GetVoice(0).MidiNote() == 76;

  kickDrive.SetDrive(0.8f);
  bool finiteOutput = true;
  float peak = 0.0f;
  uint32_t startedAtUs = System::GetUs();
  for (size_t frame = 0; frame < DSP_VALIDATION_FRAMES; ++frame) {
    float voiceLeft = 0.0f;
    float voiceRight = 0.0f;
    synthEngine.Process(voiceLeft, voiceRight);
    float kickSample = kickDrive.Process(kick.Process(frame == 0));
    float dryLeft = voiceLeft + kickSample * 0.72f;
    float dryRight = voiceRight + kickSample * 0.72f;
    float delayedLeft = 0.0f;
    float delayedRight = 0.0f;
    float outputLeft = 0.0f;
    float outputRight = 0.0f;
    processDelay(dryLeft, dryRight, delayedLeft, delayedRight);
    processReverb(delayedLeft, delayedRight, outputLeft, outputRight);

    finiteOutput = finiteOutput && isfinite(outputLeft) && isfinite(outputRight);
    float framePeak = fmaxf(fabsf(outputLeft), fabsf(outputRight));
    if (framePeak > peak) peak = framePeak;
  }
  uint32_t elapsedUs = System::GetUs() - startedAtUs;
  float availableUs = DSP_VALIDATION_FRAMES * 1000000.0f / sampleRate;

  dspWorstCaseElapsedUs = elapsedUs;
  dspWorstCaseLoadPercent = elapsedUs * 100.0f / availableUs;
  dspValidationPeak = peak;
  dspStartupValidationPassed = fourVoicesAllocated && oldestVoiceStolen &&
                               finiteOutput && peak > 0.00001f;
}
#endif

void audioCallback(AudioHandle::InterleavingInputBuffer,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t size) {
  applyControlAtBlockBoundary();

  for (size_t i = 0; i < size; i += 2) {
    bool outputEnabled = hasControlFlag(CONTROL_OUTPUT_ENABLED) && linkActive;
    bool audioTest = outputEnabled && hasControlFlag(CONTROL_AUDIO_TEST_ENABLED);
    // Keep the physical audio output at zero until a valid control snapshot
    // has explicitly enabled it. Both directions use the same slow fade, so
    // boot, soft stop and a lost link cannot introduce a discontinuity.
    float targetLinkGain = outputEnabled ? 1.0f : 0.0f;
    linkGain += (targetLinkGain - linkGain) * AUDIO_GAIN_RAMP;
    float targetMaster = outputEnabled ? activeControl.masterVolume / 100.0f : 0.0f;
    masterGain += (targetMaster - masterGain) * AUDIO_GAIN_RAMP;

    float left = 0.0f;
    float right = 0.0f;

    if (audioTest) {
      float test = testOscillator.Process() * 0.20f;
      left = test;
      right = test;
    } else {
      advanceSequencer();

      synthEngine.SetCompatibilityGate(envelopeGateIsOpen());
      float voiceLeft = 0.0f;
      float voiceRight = 0.0f;
      synthEngine.Process(voiceLeft, voiceRight);
      float kickSample = kick.Process(kickTriggerPending);
      kickTriggerPending = false;
      kickSample = processKickDrive(kickSample) * (activeControl.kickLevel / 100.0f);

      float dryLeft = voiceLeft + kickSample * 0.72f;
      float dryRight = voiceRight + kickSample * 0.72f;
      float delayedLeft = 0.0f;
      float delayedRight = 0.0f;
      processDelay(dryLeft, dryRight, delayedLeft, delayedRight);
      processReverb(delayedLeft, delayedRight, left, right);
      left *= OUTPUT_GAIN;
      right *= OUTPUT_GAIN;
    }

    float outputGain = masterGain * linkGain;
    left = clampFloat(left * outputGain, -1.0f, 1.0f);
    right = clampFloat(right * outputGain, -1.0f, 1.0f);
    out[i] = left;
    out[i + 1] = right;
  }
}

void sendStatus(uint32_t now) {
  StatusPayload payload = {};
  payload.flags = linkActive ? STATUS_LINK_ACTIVE : 0;
  if (hasControlFlag(CONTROL_TRANSPORT_RUNNING)) {
    payload.flags |= STATUS_TRANSPORT_RUNNING;
  }
  payload.currentStep = currentStep;
  payload.lastControlSequence = activeControlSequence;
  payload.acceptedPackets = acceptedPackets;
  payload.rejectedPackets = rejectedPackets;
  payload.uartRxRestartCount = uartRxRestartCount;
  payload.uartRxErrorCount = uartRxErrorCount;
  payload.uptimeMs = now;

  StatusPacket packet;
  preparePacket<StatusPacket, StatusPayload>(
    packet,
    STATUS_PACKET_TYPE,
    statusSequence++,
    payload
  );
  UartHandler::Result result = controlUart.BlockingTransmit(
    reinterpret_cast<uint8_t*>(&packet),
    sizeof(packet),
    STATUS_TX_TIMEOUT_MS
  );
  // HAL transmit timeouts can also disable receiver error interrupts. A
  // working RX alone must not prevent recovery of a broken return channel.
  if (result != UartHandler::Result::OK) uartRxRestartRequested = true;
}

void initializeDsp() {
  sampleRate = hardware.AudioSampleRate();
  synthEngine.Init(sampleRate);
  synthPatchConfigured = false;

  testOscillator.Init(sampleRate);
  testOscillator.SetWaveform(Oscillator::WAVE_SIN);
  testOscillator.SetFreq(440.0f);
  testOscillator.SetAmp(1.0f);

  initializeGlobalProcessors();
#if SYNTH_ENABLE_STARTUP_VALIDATION
  runDspStartupValidation();

  // Validation deliberately exercises every processor. Reinitialize the live
  // state so startup sounds and the first UART snapshot see clean DSP state.
  synthEngine.Init(sampleRate);
  synthPatchConfigured = false;
  initializeGlobalProcessors();
#endif
}

}  // namespace

int main(void) {
  hardware.Configure();
  hardware.Init();
  hardware.SetAudioBlockSize(AUDIO_BLOCK_SIZE);

#if !SYNTH_LINK_DIAGNOSTIC
  initializeDsp();
#endif
  uartRxRestartRequested = !startUartReceiver(false);
#if !SYNTH_LINK_DIAGNOSTIC
  hardware.StartAudio(audioCallback);
#endif

  uint32_t lastStatusMs = 0;
  uint32_t lastUartRecoveryMs = System::GetNow();
  while (true) {
    uint32_t now = System::GetNow();
#if SYNTH_LINK_DIAGNOSTIC
    // Isolate the physical UART, parser and handshake from DSP/audio IRQ load.
    // No audio is started; acknowledge snapshots from the main loop instead.
    {
      ScopedIrqBlocker block;
      if (linkActive) {
        activeControlSequence = controlSlotSequences[publishedControlSlot];
        activeControl.flags = controlSlots[publishedControlSlot].flags;
      }
    }
#endif
    if (linkActive && now - lastPacketMs > LINK_TIMEOUT_MS) {
      linkActive = false;
      uartRxRestartRequested = true;
    }

    // libDaisy does not always route circular-DMA UART errors to our callback.
    // Poll them and, while no peer has completed a frame, periodically rearm
    // only USART1. This covers either board starting first without resetting
    // the audio engine or a healthy established link.
    if (controlUart.CheckError() != 0) uartRxRestartRequested = true;
    bool bootstrapRearmDue = !linkActive &&
                             now - lastUartRecoveryMs >= UART_BOOTSTRAP_REARM_MS;
    if ((uartRxRestartRequested || bootstrapRearmDue) &&
        now - lastUartRecoveryMs >= 250) {
      lastUartRecoveryMs = now;
      uartRxRestartRequested = false;
      linkActive = false;
      if (!startUartReceiver(true)) uartRxRestartRequested = true;
    }

#if SYNTH_LINK_DIAGNOSTIC
    hardware.SetLed(linkActive || ((now / 250) % 2 != 0));
#else
    hardware.SetLed(linkActive);
#endif
    if (now - lastStatusMs >= STATUS_PERIOD_MS) {
      lastStatusMs = now;
      sendStatus(now);
    }
    System::Delay(1);
  }
}
