#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Protocollo binario condiviso da ESP32 e Daisy. Tutti i valori sono interi:
// il callback audio della Daisy non deve fare parsing di testo o allocazioni.
namespace DaisyLinkProtocol {

// Version 2 adds an explicit one-shot kick trigger and UART recovery telemetry.
// Both boards must run firmware built from the same protocol version.
constexpr uint8_t PROTOCOL_VERSION = 2;
constexpr uint8_t SYNC_0 = 0xD5;
constexpr uint8_t SYNC_1 = 0x59;
constexpr uint8_t CONTROL_PACKET_TYPE = 1;
constexpr uint8_t STATUS_PACKET_TYPE = 2;
constexpr uint32_t UART_BAUD_RATE = 115200;
constexpr int VOICE_COUNT = 2;
constexpr int STEP_COUNT = 16;

enum ControlFlag : uint8_t {
  CONTROL_OUTPUT_ENABLED = 1U << 0,
  CONTROL_TRANSPORT_RUNNING = 1U << 1,
  CONTROL_SEQUENCER_ENABLED = 1U << 2,
  CONTROL_FILTER_ENABLED = 1U << 3,
  CONTROL_PREVIEW_ENABLED = 1U << 4,
  CONTROL_AUDIO_TEST_ENABLED = 1U << 5
};

enum StatusFlag : uint8_t {
  STATUS_LINK_ACTIVE = 1U << 0,
  STATUS_TRANSPORT_RUNNING = 1U << 1
};

#if defined(__GNUC__)
#define DAISY_LINK_PACKED __attribute__((packed))
#else
#define DAISY_LINK_PACKED
#endif

struct DAISY_LINK_PACKED PacketHeader {
  uint8_t sync0;
  uint8_t sync1;
  uint8_t version;
  uint8_t type;
  uint16_t payloadSize;
  uint16_t sequence;
};

struct DAISY_LINK_PACKED ControlPayload {
  uint8_t flags;
  uint8_t bpm;
  uint8_t sequenceLength;
  uint8_t selectedStep;
  uint8_t previewVoice;
  uint8_t masterVolume;

  uint8_t waveform[VOICE_COUNT];
  uint8_t voiceLevel[VOICE_COUNT];
  int8_t tuneSemitones[VOICE_COUNT];
  int8_t detuneCents[VOICE_COUNT];
  uint8_t wavefold[VOICE_COUNT];

  uint16_t voiceEnabledMask[VOICE_COUNT];
  uint8_t stepMidiNote[VOICE_COUNT][STEP_COUNT];

  uint16_t attackMs;
  uint16_t decayMs;
  uint8_t sustainPercent;
  uint16_t releaseMs;

  uint8_t filterType;
  uint16_t filterCutoffHz;

  uint16_t kickEnabledMask;
  uint16_t kickTriggerSequence;
  uint8_t kickLevel;
  uint8_t kickPitchHz;
  uint16_t kickDecayMs;
  uint16_t kickSweepHz;
  uint8_t kickClick;
  uint8_t kickDrive;

  uint8_t delayMix;
  uint16_t delayTimeMs;
  uint8_t delayFeedback;
  uint8_t reverbMix;
  uint8_t reverbRoom;
};

struct DAISY_LINK_PACKED StatusPayload {
  uint8_t flags;
  uint8_t currentStep;
  uint16_t lastControlSequence;
  uint32_t acceptedPackets;
  uint32_t rejectedPackets;
  uint32_t uartRxRestartCount;
  uint32_t uartRxErrorCount;
  uint32_t uptimeMs;
};

struct DAISY_LINK_PACKED ControlPacket {
  PacketHeader header;
  ControlPayload payload;
  uint16_t checksum;
};

struct DAISY_LINK_PACKED StatusPacket {
  PacketHeader header;
  StatusPayload payload;
  uint16_t checksum;
};

static_assert(sizeof(PacketHeader) == 8, "Unexpected Daisy link header size");
static_assert(sizeof(ControlPacket) < 128, "Control packet must stay small");
static_assert(sizeof(StatusPacket) < 64, "Status packet must stay small");

inline uint16_t crc16Ccitt(const uint8_t* data, size_t size) {
  uint16_t crc = 0xFFFFU;

  for (size_t i = 0; i < size; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000U) != 0U
              ? (uint16_t)((crc << 1) ^ 0x1021U)
              : (uint16_t)(crc << 1);
    }
  }

  return crc;
}

template <typename Packet, typename Payload>
inline void preparePacket(Packet& packet,
                          uint8_t type,
                          uint16_t sequence,
                          const Payload& payload) {
  packet.header.sync0 = SYNC_0;
  packet.header.sync1 = SYNC_1;
  packet.header.version = PROTOCOL_VERSION;
  packet.header.type = type;
  packet.header.payloadSize = (uint16_t)sizeof(Payload);
  packet.header.sequence = sequence;
  memcpy(&packet.payload, &payload, sizeof(Payload));
  packet.checksum = crc16Ccitt(
    reinterpret_cast<const uint8_t*>(&packet),
    offsetof(Packet, checksum)
  );
}

template <typename Packet, typename Payload>
inline bool isPacketValid(const Packet& packet, uint8_t expectedType) {
  if (packet.header.sync0 != SYNC_0 ||
      packet.header.sync1 != SYNC_1 ||
      packet.header.version != PROTOCOL_VERSION ||
      packet.header.type != expectedType ||
      packet.header.payloadSize != sizeof(Payload)) {
    return false;
  }

  uint16_t expected = crc16Ccitt(
    reinterpret_cast<const uint8_t*>(&packet),
    offsetof(Packet, checksum)
  );
  return packet.checksum == expected;
}

}  // namespace DaisyLinkProtocol

#undef DAISY_LINK_PACKED
