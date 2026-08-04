#include "daisy_seed.h"
#include "daisysp.h"

#include <math.h>
#include <string.h>

#include "../TheSynthProject/DaisyLinkProtocol.h"

using namespace daisy;
using namespace daisysp;
using namespace DaisyLinkProtocol;

namespace {

constexpr size_t AUDIO_BLOCK_SIZE = 48;
constexpr size_t UART_DMA_BUFFER_SIZE = 256;
constexpr uint32_t LINK_TIMEOUT_MS = 500;
constexpr uint32_t STATUS_PERIOD_MS = 40;
constexpr uint32_t UART_RECOVERY_PERIOD_MS = 1000;
constexpr float OUTPUT_GAIN = 0.30f;
constexpr size_t MAX_DELAY_SAMPLES = 48000U * 6U / 10U;

DaisySeed hardware;
UartHandler controlUart;

Oscillator oscillators[VOICE_COUNT];
Oscillator testOscillator;
Wavefolder wavefolders[VOICE_COUNT];
Adsr ampEnvelope;
Svf filter;
AnalogBassDrum kick;
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

float clampFloat(float value, float minimum, float maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}

uint8_t clampByte(uint8_t value, uint8_t maximum) {
  return value > maximum ? maximum : value;
}

float midiToFrequency(float note) {
  return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
}

bool hasControlFlag(uint8_t flag) {
  return (activeControl.flags & flag) != 0U;
}

bool isStepEnabled(uint16_t mask, uint8_t step) {
  return (mask & (uint16_t)(1U << step)) != 0U;
}

uint8_t oscillatorWaveform(uint8_t espShape) {
  switch (espShape) {
    case 0: return Oscillator::WAVE_SIN;
    case 1: return Oscillator::WAVE_POLYBLEP_SQUARE;
    case 2: return Oscillator::WAVE_POLYBLEP_TRI;
    case 3: return Oscillator::WAVE_POLYBLEP_SAW;
    case 4: return Oscillator::WAVE_RAMP;
    case 5: return Oscillator::WAVE_POLYBLEP_SQUARE;
    default: return Oscillator::WAVE_SIN;
  }
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

class ControlPacketParser {
 public:
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
      rejectedPackets++;
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

void updateVoiceFrequency(uint8_t voice) {
  uint8_t step = currentStep < STEP_COUNT ? currentStep : 0;
  float note = activeControl.stepMidiNote[voice][step]
             + activeControl.tuneSemitones[voice]
             + activeControl.detuneCents[voice] / 100.0f;
  oscillators[voice].SetFreq(midiToFrequency(clampFloat(note, 0.0f, 127.0f)));
}

bool anyVoiceEnabledAtCurrentStep() {
  for (uint8_t voice = 0; voice < VOICE_COUNT; ++voice) {
    if (isStepEnabled(activeControl.voiceEnabledMask[voice], currentStep)) {
      return true;
    }
  }
  return false;
}

void triggerCurrentStep() {
  for (uint8_t voice = 0; voice < VOICE_COUNT; ++voice) {
    updateVoiceFrequency(voice);
  }

  if (hasControlFlag(CONTROL_SEQUENCER_ENABLED) && anyVoiceEnabledAtCurrentStep()) {
    ampEnvelope.Retrigger(false);
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
  activeControl.previewVoice = clampByte(activeControl.previewVoice, VOICE_COUNT - 1);

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

  updateClockSettings();

  for (uint8_t voice = 0; voice < VOICE_COUNT; ++voice) {
    oscillators[voice].SetWaveform(oscillatorWaveform(activeControl.waveform[voice]));
    oscillators[voice].SetPw(activeControl.waveform[voice] == 5 ? 0.25f : 0.5f);
    wavefolders[voice].SetGain(1.0f + activeControl.wavefold[voice] * 0.03f);
    updateVoiceFrequency(voice);
  }

  ampEnvelope.SetAttackTime(activeControl.attackMs / 1000.0f);
  ampEnvelope.SetDecayTime(activeControl.decayMs / 1000.0f);
  ampEnvelope.SetSustainLevel(clampFloat(activeControl.sustainPercent / 100.0f, 0.0f, 1.0f));
  ampEnvelope.SetReleaseTime(activeControl.releaseMs / 1000.0f);

  float cutoff = clampFloat(activeControl.filterCutoffHz, 40.0f, sampleRate / 3.0f);
  filter.SetFreq(cutoff);
  filter.SetRes(0.15f);
  filter.SetDrive(0.0f);

  kick.SetFreq(clampFloat(activeControl.kickPitchHz, 25.0f, 120.0f));
  kick.SetDecay(clampFloat((activeControl.kickDecayMs - 80.0f) / 1120.0f, 0.0f, 1.0f));
  kick.SetTone(activeControl.kickClick / 100.0f);
  kick.SetAttackFmAmount(clampFloat(activeControl.kickSweepHz / 260.0f, 0.0f, 1.0f));
  kick.SetSelfFmAmount(0.25f);
  kick.SetAccent(0.8f);
  kickDrive.SetDrive(activeControl.kickDrive / 100.0f);

  reverb.SetFeedback(0.72f + activeControl.reverbRoom * 0.0023f);
  reverb.SetLpFreq(12000.0f);

  bool previewWasEnabled = (previous.flags & CONTROL_PREVIEW_ENABLED) != 0U;
  bool previewEnabled = hasControlFlag(CONTROL_PREVIEW_ENABLED);
  bool previewChanged = previous.previewVoice != activeControl.previewVoice ||
                        previous.selectedStep != activeControl.selectedStep;
  if (previewEnabled && (!previewWasEnabled || previewChanged)) {
    ampEnvelope.Retrigger(false);
  }

  if (activeControl.kickTriggerSequence != previous.kickTriggerSequence &&
      activeControl.kickLevel > 0) {
    kickTriggerPending = true;
  }
}

void advanceSequencer() {
  if (!hasControlFlag(CONTROL_OUTPUT_ENABLED) ||
      !hasControlFlag(CONTROL_TRANSPORT_RUNNING)) {
    return;
  }

  stepSample++;
  if (stepSample < samplesPerStep) return;

  stepSample = 0;
  currentStep = (uint8_t)((currentStep + 1U) % activeControl.sequenceLength);
  triggerCurrentStep();
}

bool voiceIsAudible(uint8_t voice) {
  if (hasControlFlag(CONTROL_PREVIEW_ENABLED)) {
    return voice == activeControl.previewVoice;
  }
  if (!hasControlFlag(CONTROL_SEQUENCER_ENABLED)) return false;
  return isStepEnabled(activeControl.voiceEnabledMask[voice], currentStep);
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

float renderVoices() {
  float mixed = 0.0f;
  float totalLevel = 0.0f;

  for (uint8_t voice = 0; voice < VOICE_COUNT; ++voice) {
    float raw = oscillators[voice].Process();
    if (!voiceIsAudible(voice)) continue;

    float level = clampFloat(activeControl.voiceLevel[voice] / 100.0f, 0.0f, 1.0f);
    mixed += wavefolders[voice].Process(raw) * level;
    totalLevel += level;
  }

  if (totalLevel > 1.0f) mixed /= totalLevel;
  return mixed;
}

float processFilter(float input) {
  if (!hasControlFlag(CONTROL_FILTER_ENABLED)) return input;

  filter.Process(input);
  switch (activeControl.filterType) {
    case 1: return filter.High();
    case 2: return filter.Band();
    default: return filter.Low();
  }
}

float processDelay(float input) {
  float target = activeControl.delayTimeMs * sampleRate / 1000.0f;
  target = clampFloat(target, 1.0f, MAX_DELAY_SAMPLES - 2.0f);
  delaySamples += (target - delaySamples) * 0.00035f;
  delayLine.SetDelay(delaySamples);

  float delayed = delayLine.Read();
  float feedback = clampFloat(activeControl.delayFeedback / 100.0f, 0.0f, 0.90f);
  delayLine.Write(clampFloat(input + delayed * feedback, -1.0f, 1.0f));

  float mix = activeControl.delayMix / 100.0f;
  return input + (delayed - input) * mix;
}

void processReverb(float input, float& left, float& right) {
  float wetLeft = 0.0f;
  float wetRight = 0.0f;
  reverb.Process(input, input, &wetLeft, &wetRight);

  float mix = activeControl.reverbMix / 100.0f;
  left = input + (wetLeft - input) * mix;
  right = input + (wetRight - input) * mix;
}

void audioCallback(AudioHandle::InterleavingInputBuffer,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t size) {
  applyControlAtBlockBoundary();

  for (size_t i = 0; i < size; i += 2) {
    bool outputEnabled = hasControlFlag(CONTROL_OUTPUT_ENABLED) && linkActive;
    bool audioTest = outputEnabled && hasControlFlag(CONTROL_AUDIO_TEST_ENABLED);
    float targetLinkGain = linkActive ? 1.0f : 0.0f;
    linkGain += (targetLinkGain - linkGain) * 0.0015f;

    float left = 0.0f;
    float right = 0.0f;

    if (audioTest) {
      float test = testOscillator.Process() * 0.20f;
      left = test;
      right = test;
    } else {
      advanceSequencer();

      float gate = ampEnvelope.Process(envelopeGateIsOpen());
      float targetMaster = activeControl.masterVolume / 100.0f;
      masterGain += (targetMaster - masterGain) * 0.0015f;

      float voices = processFilter(renderVoices() * gate * masterGain);
      float kickSample = kick.Process(kickTriggerPending);
      kickTriggerPending = false;
      kickSample = kickDrive.Process(kickSample) * (activeControl.kickLevel / 100.0f);

      float effected = processDelay(voices + kickSample * 0.72f);
      processReverb(effected, left, right);
      left *= OUTPUT_GAIN;
      right *= OUTPUT_GAIN;
    }

    left = clampFloat(left * linkGain, -1.0f, 1.0f);
    right = clampFloat(right * linkGain, -1.0f, 1.0f);
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
  controlUart.BlockingTransmit(
    reinterpret_cast<uint8_t*>(&packet),
    sizeof(packet),
    5
  );
}

void initializeDsp() {
  sampleRate = hardware.AudioSampleRate();

  for (uint8_t voice = 0; voice < VOICE_COUNT; ++voice) {
    oscillators[voice].Init(sampleRate);
    oscillators[voice].SetWaveform(Oscillator::WAVE_SIN);
    oscillators[voice].SetFreq(220.0f);
    oscillators[voice].SetAmp(1.0f);
    wavefolders[voice].Init();
    wavefolders[voice].SetGain(1.0f);
  }

  testOscillator.Init(sampleRate);
  testOscillator.SetWaveform(Oscillator::WAVE_SIN);
  testOscillator.SetFreq(440.0f);
  testOscillator.SetAmp(1.0f);

  ampEnvelope.Init(sampleRate);
  ampEnvelope.SetAttackTime(0.01f);
  ampEnvelope.SetDecayTime(0.08f);
  ampEnvelope.SetSustainLevel(0.68f);
  ampEnvelope.SetReleaseTime(0.32f);

  filter.Init(sampleRate);
  filter.SetFreq(2400.0f);
  filter.SetRes(0.15f);

  kick.Init(sampleRate);
  kickDrive.Init();
  delayLine.Init();
  delayLine.SetDelay(delaySamples);
  reverb.Init(sampleRate);
  reverb.SetFeedback(0.82f);
  reverb.SetLpFreq(12000.0f);
}

}  // namespace

int main(void) {
  hardware.Configure();
  hardware.Init();
  hardware.SetAudioBlockSize(AUDIO_BLOCK_SIZE);

  initializeDsp();
  startUartReceiver(false);
  hardware.StartAudio(audioCallback);

  uint32_t lastStatusMs = 0;
  uint32_t lastUartRecoveryMs = System::GetNow();
  while (true) {
    uint32_t now = System::GetNow();
    if (linkActive && now - lastPacketMs > LINK_TIMEOUT_MS) {
      linkActive = false;
      uartRxRestartRequested = true;
    }

    bool periodicRecoveryDue = !linkActive &&
                               now - lastUartRecoveryMs >= UART_RECOVERY_PERIOD_MS;
    if (uartRxRestartRequested || periodicRecoveryDue) {
      lastUartRecoveryMs = now;
      uartRxRestartRequested = !startUartReceiver(true);
    }

    hardware.SetLed(linkActive);
    if (now - lastStatusMs >= STATUS_PERIOD_MS) {
      lastStatusMs = now;
      sendStatus(now);
    }
    System::Delay(1);
  }
}
