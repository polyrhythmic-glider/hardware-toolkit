#pragma once

#include "DaisyLinkProtocol.h"

namespace DaisyBridge {

using namespace DaisyLinkProtocol;

constexpr uint32_t SEND_PERIOD_MS = 20;
constexpr uint32_t STATUS_TIMEOUT_MS = 500;

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

uint16_t makeVoiceMask(int voice) {
  uint16_t mask = 0;
  for (int step = 0; step < STEP_COUNT; ++step) {
    if (isSequencerStepEnabled(voice, step)) mask |= (uint16_t)(1U << step);
  }
  return mask;
}

uint16_t makeKickMask() {
  uint16_t mask = 0;
  for (int step = 0; step < STEP_COUNT; ++step) {
    if (isKickStepEnabled(step)) mask |= (uint16_t)(1U << step);
  }
  return mask;
}

bool shouldEnableDaisyOutput() {
  if (uiMode == MODE_BOOT || uiMode == MODE_ABOUT) return false;
  if (isAudioTestActive() || isVoicePreviewActive()) return true;
  return transportRunning && (sequencerEnabled || isKickTrackAudible());
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
  payload.previewVoice = (uint8_t)clampInt(getCurrentWaveVoiceIndex(), 0, VOICE_COUNT - 1);
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
