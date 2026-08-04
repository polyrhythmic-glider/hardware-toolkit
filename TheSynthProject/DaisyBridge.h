#pragma once

#include "DaisyLinkProtocol.h"

namespace DaisyBridge {

using namespace DaisyLinkProtocol;

constexpr uint32_t SEND_PERIOD_MS = 40;
constexpr uint32_t STATUS_TIMEOUT_MS = 500;
constexpr size_t UART_TX_BUFFER_SIZE = 768;

static uint16_t controlSequence = 0;
static uint32_t lastSendMs = 0;
static uint32_t lastStatusMs = 0;
static bool connected = false;
static bool returnLinkActive = false;
static uint32_t sentControlPackets = 0;
static uint32_t droppedControlPackets = 0;
static uint32_t receivedStatusPackets = 0;
static uint32_t rejectedStatusPackets = 0;
static uint32_t remoteAcceptedPackets = 0;
static uint32_t remoteRejectedPackets = 0;
static uint32_t remoteUartRxRestartCount = 0;
static uint32_t remoteUartRxErrorCount = 0;
static uint16_t remoteControlSequence = 0;
static bool debugStateInitialized = false;
static bool lastDebugForwardLink = false;
static bool lastDebugReturnLink = false;
static uint32_t lastDebugReportMs = 0;

class StatusPacketParser {
 public:
  bool push(uint8_t byte, StatusPacket& packet) {
    if (index_ == 0 && byte != SYNC_0) return false;

    if (index_ == 1 && byte != SYNC_1) {
      index_ = byte == SYNC_0 ? 1U : 0U;
      buffer_[0] = SYNC_0;
      return false;
    }

    buffer_[index_++] = byte;
    if (index_ != sizeof(StatusPacket)) return false;

    memcpy(&packet, buffer_, sizeof(packet));
    index_ = 0;
    bool valid = isPacketValid<StatusPacket, StatusPayload>(packet, STATUS_PACKET_TYPE);
    if (valid) receivedStatusPackets++;
    else rejectedStatusPackets++;
    return valid;
  }

 private:
  uint8_t buffer_[sizeof(StatusPacket)] = {};
  size_t index_ = 0;
};

static StatusPacketParser statusParser;

uint64_t makeVoiceMask(int voice) {
  uint64_t mask = 0;
  for (int step = 0; step < STEP_COUNT; ++step) {
    if (isSequencerStepEnabled(voice, step)) mask |= (uint64_t)(1ULL << step);
  }
  return mask;
}

uint64_t makeKickMask() {
  uint64_t mask = 0;
  for (int step = 0; step < STEP_COUNT; ++step) {
    if (isKickStepEnabled(step)) mask |= (uint64_t)(1ULL << step);
  }
  return mask;
}

bool shouldEnableDaisyOutput() {
  if (uiMode == MODE_BOOT || uiMode == MODE_ABOUT) return false;
  if (isAudioTestActive() || isVoicePreviewActive()) return true;
  return transportRunning && (sequencerEnabled || isKickTrackAudible());
}

uint8_t mapModulationDestinationToDaisy(int destination) {
  switch (destination) {
    case MOD_DEST_AMP_LEVEL: return DSP_DEST_AMP;
    case MOD_DEST_FILTER_CUTOFF: return DSP_DEST_FILTER_CUTOFF;
    case MOD_DEST_FILTER_RESONANCE: return DSP_DEST_FILTER_RESONANCE;
    case MOD_DEST_OSC1_PITCH: return DSP_DEST_OSC1_PITCH;
    case MOD_DEST_OSC2_PITCH: return DSP_DEST_OSC2_PITCH;
    case MOD_DEST_OSC3_PITCH: return DSP_DEST_OSC3_PITCH;
    case MOD_DEST_ALL_OSC_PITCH: return DSP_DEST_ALL_OSC_PITCH;
    case MOD_DEST_OSC1_LEVEL: return DSP_DEST_OSC1_LEVEL;
    case MOD_DEST_OSC2_LEVEL: return DSP_DEST_OSC2_LEVEL;
    case MOD_DEST_OSC3_LEVEL: return DSP_DEST_OSC3_LEVEL;
    case MOD_DEST_NOISE_LEVEL: return DSP_DEST_NOISE_LEVEL;
    case MOD_DEST_OSC1_PULSE_WIDTH: return DSP_DEST_OSC1_PULSE_WIDTH;
    case MOD_DEST_OSC2_PULSE_WIDTH: return DSP_DEST_OSC2_PULSE_WIDTH;
    case MOD_DEST_OSC3_PULSE_WIDTH: return DSP_DEST_OSC3_PULSE_WIDTH;
    case MOD_DEST_OSC1_WAVEFOLD: return DSP_DEST_OSC1_WAVEFOLD;
    case MOD_DEST_OSC2_WAVEFOLD: return DSP_DEST_OSC2_WAVEFOLD;
    case MOD_DEST_OSC3_WAVEFOLD: return DSP_DEST_OSC3_WAVEFOLD;
    case MOD_DEST_PAN: return DSP_DEST_PAN;
    case MOD_DEST_DELAY_MIX: return DSP_DEST_DELAY_MIX;
    case MOD_DEST_REVERB_MIX: return DSP_DEST_REVERB_MIX;
    default: return DSP_DESTINATION_COUNT;
  }
}

void fillControlPayload(ControlPayload& payload) {
  memset(&payload, 0, sizeof(payload));

  if (shouldEnableDaisyOutput()) payload.flags |= CONTROL_OUTPUT_ENABLED;
  if (transportRunning) payload.flags |= CONTROL_TRANSPORT_RUNNING;
  if (sequencerEnabled) payload.flags |= CONTROL_SEQUENCER_ENABLED;
  if (filterEnabled) payload.flags |= CONTROL_FILTER_ENABLED;
  if (isVoicePreviewActive()) payload.flags |= CONTROL_PREVIEW_ENABLED;
  if (isAudioTestActive()) payload.flags |= CONTROL_AUDIO_TEST_ENABLED;

  payload.bpm = (uint8_t)clampInt(sequencerBpm, 40, 240);
  payload.sequenceLength = (uint8_t)clampInt(getSequencerLength(), 1, STEP_COUNT);
  payload.selectedStep = (uint8_t)clampInt(sequencerSelectedStep, 0, STEP_COUNT - 1);
  payload.previewVoice = (uint8_t)clampInt(
    getCurrentWaveVoiceIndex(), 0, OSCILLATOR_PARAMETER_COUNT - 1
  );
  payload.masterVolume = (uint8_t)clampInt(volume, 0, 100);

  for (int voice = 0; voice < VOICE_COUNT; ++voice) {
    payload.waveform[voice] = (uint8_t)clampInt(waveVoiceShape[voice], 0, WAVE_SHAPE_COUNT - 1);
    payload.voiceLevel[voice] = (uint8_t)clampInt(waveVoiceLevels[voice], 0, 100);
    payload.tuneSemitones[voice] = (int8_t)clampInt(waveVoiceTune[voice], -24, 24);
    payload.detuneCents[voice] = (int8_t)clampInt(waveVoiceDetune[voice], -50, 50);
    payload.wavefold[voice] = (uint8_t)clampInt(waveVoiceFold[voice], 0, 100);
    payload.voiceEnabledMask[voice] = makeVoiceMask(voice);

    for (int step = 0; step < STEP_COUNT; ++step) {
      payload.stepMidiNote[voice][step] = (uint8_t)clampMidiPitch(
        getSequencerStepPitch(voice, step)
      );
    }
  }

  const AdsrParameters& envelope = envelopeParameters[ENVELOPE_ENV1];
  payload.attackMs = (uint16_t)clampInt(envelope.attackMs, 0, 500);
  payload.decayMs = (uint16_t)clampInt(envelope.decayMs, 0, 2000);
  payload.sustainPercent = (uint8_t)clampInt(envelope.sustainPercent, 0, 100);
  payload.releaseMs = (uint16_t)clampInt(envelope.releaseMs, 0, 3000);

  payload.filterType = (uint8_t)clampInt(filterType, 0, FILTER_TYPE_COUNT - 1);
  payload.filterCutoffHz = (uint16_t)clampInt(
    filterCutoff,
    FILTER_MIN_CUTOFF_HZ,
    FILTER_MAX_CUTOFF_HZ
  );

  payload.kickEnabledMask = makeKickMask();
  payload.kickTriggerSequence = kickTriggerSequence;
  payload.kickLevel = (uint8_t)clampInt(kickLevel, 0, 100);
  payload.kickPitchHz = (uint8_t)clampInt(kickPitch, 25, 120);
  payload.kickDecayMs = (uint16_t)clampInt(kickDecay, 80, 1200);
  payload.kickSweepHz = (uint16_t)clampInt(kickSweep, 0, 260);
  payload.kickAttack = (uint8_t)clampInt(kickAttack, 0, 100);
  payload.kickDrive = (uint8_t)clampInt(kickDrive, 0, 100);

  payload.delayMix = (uint8_t)clampInt(delayMix, 0, 100);
  payload.delayTimeMs = (uint16_t)clampInt(delayTimeMs, 20, DELAY_MAX_MS);
  payload.delayFeedback = (uint8_t)clampInt(delayFeedback, 0, 90);
  payload.reverbMix = (uint8_t)clampInt(reverbMix, 0, 100);
  payload.reverbRoom = (uint8_t)clampInt(reverbRoom, 0, 95);

  for (int oscillator = 0; oscillator < OSCILLATOR_PARAMETER_COUNT; ++oscillator) {
    DaisyLinkProtocol::OscillatorControlParameters& target =
      payload.oscillators[oscillator];
    target.flags = oscillatorEnabled[oscillator] ? 1U : 0U;
    target.waveform = (uint8_t)clampInt(waveVoiceShape[oscillator], 0, WAVE_SHAPE_COUNT - 1);
    target.levelPercent = (uint8_t)clampInt(waveVoiceLevels[oscillator], 0, 100);
    target.octave = (int8_t)clampInt(oscillatorOctave[oscillator], -4, 4);
    target.semitoneTune = (int8_t)clampInt(waveVoiceTune[oscillator], -48, 48);
    target.fineTuneCents = (int8_t)clampInt(waveVoiceDetune[oscillator], -100, 100);
    target.phasePercent = (uint8_t)clampInt(oscillatorPhasePercent[oscillator], 0, 100);
    target.pulseWidthPercent = (uint8_t)clampInt(oscillatorPulseWidthPercent[oscillator], 1, 99);
    target.wavefoldPercent = (uint8_t)clampInt(waveVoiceFold[oscillator], 0, 100);
    target.panPercent = (int8_t)clampInt(oscillatorPanPercent[oscillator], -100, 100);
    target.keyTrackingPercent = (uint8_t)clampInt(oscillatorKeyTrackingPercent[oscillator], 0, 100);
    target.modulationAmountPercent = (int8_t)clampInt(
      oscillatorModulationAmountPercent[oscillator], -100, 100
    );
  }

  for (int envelope = 0; envelope < ENVELOPE_PARAMETER_COUNT; ++envelope) {
    DaisyLinkProtocol::EnvelopeControlParameters& target =
      payload.envelopes[envelope];
    target.flags = (envelopeEnabled[envelope] ? 1U : 0U) |
                   ((uint8_t)clampInt(envelopeRetriggerMode[envelope], 0, 2) << 1);
    target.attackMs = (uint16_t)clampInt(envelopeParameters[envelope].attackMs, 0, 30000);
    target.decayMs = (uint16_t)clampInt(envelopeParameters[envelope].decayMs, 0, 30000);
    target.sustainPercent = (uint8_t)clampInt(
      envelopeParameters[envelope].sustainPercent, 0, 100
    );
    target.releaseMs = (uint16_t)clampInt(envelopeParameters[envelope].releaseMs, 0, 30000);
    target.amountPercent = (int8_t)clampInt(envelopeAmountPercent[envelope], -100, 100);
    target.velocitySensitivityPercent = (uint8_t)clampInt(
      envelopeVelocitySensitivityPercent[envelope], 0, 100
    );
  }

  for (int lfo = 0; lfo < LFO_PARAMETER_COUNT; ++lfo) {
    DaisyLinkProtocol::LfoControlParameters& target = payload.lfos[lfo];
    const ::LfoControlParameters& source = lfoParameters[lfo];
    target.flags = (source.enabled ? 1U : 0U) |
                   (source.bipolar ? 1U << 1 : 0U) |
                   (source.retriggerOnNoteOn ? 1U << 2 : 0U) |
                   (source.freeRunning ? 1U << 3 : 0U) |
                   (source.tempoSync ? 1U << 4 : 0U);
    target.waveform = (uint8_t)clampInt(source.waveform, 0, LFO_WAVE_COUNT - 1);
    target.frequencyCentiHz = (uint16_t)clampInt(source.frequencyCentiHz, 0, 20000);
    target.phasePercent = (uint8_t)clampInt(source.phasePercent, 0, 100);
    target.amountPercent = (int8_t)clampInt(source.amountPercent, -100, 100);
    target.tempoDivision = source.tempoDivision;
  }

  int packedRoute = 0;
  for (int source = 0; source < MOD_SOURCE_COUNT && packedRoute < MODULATION_ROUTE_COUNT; ++source) {
    for (int destination = 0;
         destination < MOD_DESTINATION_COUNT && packedRoute < MODULATION_ROUTE_COUNT;
         ++destination) {
      const ModRouteSettings& route = modulationRoutes[source][destination];
      uint8_t daisyDestination = mapModulationDestinationToDaisy(destination);
      if (!route.enabled || daisyDestination >= DSP_DESTINATION_COUNT) continue;

      DaisyLinkProtocol::ModulationRouteParameters& target =
        payload.modulationRoutes[packedRoute++];
      target.flags = 1U;
      target.source = (uint8_t)source;
      target.destination = daisyDestination;
      target.amountPercent = (int8_t)clampModulationAmount(route.amountPercent);
    }
  }

  payload.filterResonancePercent = (uint8_t)clampInt(filterResonance, 0, 100);
  payload.noiseLevelPercent = (uint8_t)clampInt(synthNoiseLevel, 0, 100);
  payload.voicePanPercent = (int8_t)clampInt(synthPan, -100, 100);
}

void sendControl() {
  ControlPayload payload;
  fillControlPayload(payload);

  ControlPacket packet;
  preparePacket<ControlPacket, ControlPayload>(
    packet,
    CONTROL_PACKET_TYPE,
    controlSequence++,
    payload
  );

  if (Serial2.availableForWrite() < (int)sizeof(packet)) {
    droppedControlPackets++;
    return;
  }

  size_t written = Serial2.write(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  if (written == sizeof(packet)) sentControlPackets++;
  else droppedControlPackets++;
}

void acceptStatus(const StatusPacket& packet, uint32_t now) {
  StatusPayload payload;
  memcpy(&payload, &packet.payload, sizeof(payload));

  lastStatusMs = now;
  returnLinkActive = true;
  connected = (payload.flags & STATUS_LINK_ACTIVE) != 0U;
  remoteAcceptedPackets = payload.acceptedPackets;
  remoteRejectedPackets = payload.rejectedPackets;
  remoteUartRxRestartCount = payload.uartRxRestartCount;
  remoteUartRxErrorCount = payload.uartRxErrorCount;
  remoteControlSequence = payload.lastControlSequence;

  int length = getSequencerLength();
  if (length > 0) {
    sequencerCurrentStep = clampInt(payload.currentStep, 0, length - 1);
    if (sequencerAutoScroll && transportRunning) {
      syncSequencerPageToStep(sequencerCurrentStep);
    }
  }
}

void pollStatus(uint32_t now) {
  while (Serial2.available() > 0) {
    StatusPacket packet;
    if (statusParser.push((uint8_t)Serial2.read(), packet)) {
      acceptStatus(packet, now);
    }
  }

  if (now - lastStatusMs > STATUS_TIMEOUT_MS) {
    connected = false;
    returnLinkActive = false;
  }
}

void reportLinkDebug(uint32_t now, bool force = false) {
  if (!isSerialDiagnosticsEnabled()) return;

  bool stateChanged = !debugStateInitialized ||
                      connected != lastDebugForwardLink ||
                      returnLinkActive != lastDebugReturnLink;
  bool periodicReportDue = now - lastDebugReportMs >= 2000;
  if (!force && !stateChanged && !periodicReportDue) return;

  Serial.print("[DAISY][LINK] ESP->Daisy=");
  Serial.print(connected ? "OK" : "WAIT");
  Serial.print(" Daisy->ESP=");
  Serial.print(returnLinkActive ? "OK" : "WAIT");
  Serial.print(" tx/drop=");
  Serial.print(sentControlPackets);
  Serial.print("/");
  Serial.print(droppedControlPackets);
  Serial.print(" rx/bad=");
  Serial.print(receivedStatusPackets);
  Serial.print("/");
  Serial.print(rejectedStatusPackets);
  Serial.print(" daisy accepted/rejected=");
  Serial.print(remoteAcceptedPackets);
  Serial.print("/");
  Serial.print(remoteRejectedPackets);
  Serial.print(" rx-restart/error=");
  Serial.print(remoteUartRxRestartCount);
  Serial.print("/");
  Serial.print(remoteUartRxErrorCount);
  Serial.print(" seq=");
  Serial.print(remoteControlSequence);
  if (returnLinkActive && !connected) {
    if (remoteRejectedPackets == 0) {
      Serial.print(" hint=DAISY RX RECOVERS AUTOMATICALLY; IF WAIT PERSISTS CHECK ESP TX2 -> DAISY D14/RX");
    } else {
      Serial.print(" hint=DAISY REJECTS DATA: REFLASH MATCHING FIRMWARE / CHECK UART");
    }
  }
  Serial.println();

  debugStateInitialized = true;
  lastDebugForwardLink = connected;
  lastDebugReturnLink = returnLinkActive;
  lastDebugReportMs = now;
}

void begin() {
  // A 64-step control snapshot is larger than the ESP32 UART FIFO. Enable a
  // ring buffer before begin() so the complete packet can be queued atomically.
  Serial2.setTxBufferSize(UART_TX_BUFFER_SIZE);
  Serial2.begin(
    UART_BAUD_RATE,
    SERIAL_8N1,
    DAISY_LINK_RX,
    DAISY_LINK_TX
  );
  lastSendMs = 0;
  lastStatusMs = millis();
  connected = false;
  returnLinkActive = false;
  sentControlPackets = 0;
  droppedControlPackets = 0;
  receivedStatusPackets = 0;
  rejectedStatusPackets = 0;
  remoteAcceptedPackets = 0;
  remoteRejectedPackets = 0;
  remoteUartRxRestartCount = 0;
  remoteUartRxErrorCount = 0;
  debugStateInitialized = false;
  lastDebugReportMs = 0;

  if (isSerialDiagnosticsEnabled()) {
    Serial.print("[DAISY][UART] 115200 8N1 ESP RX2/GPIO");
    Serial.print(DAISY_LINK_RX);
    Serial.print(" <- Daisy D13/TX, ESP TX2/GPIO");
    Serial.print(DAISY_LINK_TX);
    Serial.println(" -> Daisy D14/RX");
  }

  sendControl();
}

void tick() {
  uint32_t now = millis();
  pollStatus(now);

  if (now - lastSendMs >= SEND_PERIOD_MS) {
    lastSendMs = now;
    sendControl();
  }

  reportLinkDebug(now);
}

bool isConnected() {
  return connected;
}

bool isReturnLinkActive() {
  return returnLinkActive;
}

uint32_t getSentControlPacketCount() {
  return sentControlPackets;
}

uint32_t getDroppedControlPacketCount() {
  return droppedControlPackets;
}

uint32_t getReceivedStatusPacketCount() {
  return receivedStatusPackets;
}

uint32_t getRejectedStatusPacketCount() {
  return rejectedStatusPackets;
}

uint32_t getAcceptedPacketCount() {
  return remoteAcceptedPackets;
}

uint32_t getRejectedPacketCount() {
  return remoteRejectedPackets;
}

uint32_t getUartRxRestartCount() {
  return remoteUartRxRestartCount;
}

uint32_t getUartRxErrorCount() {
  return remoteUartRxErrorCount;
}

uint16_t getRemoteControlSequence() {
  return remoteControlSequence;
}

}  // namespace DaisyBridge
