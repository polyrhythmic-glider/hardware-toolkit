#include "../DaisyLinkHealth.h"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace DaisyLinkProtocol;

static void acknowledge(LinkHealth& health, uint16_t seq, uint32_t now,
                        uint32_t uptime) {
  health.sent(seq, now);
  StatusPayload status = {};
  status.flags = STATUS_LINK_ACTIVE;
  status.acceptedPackets = 10;
  status.lastControlSequence = seq;
  status.uptimeMs = uptime;
  health.status(status, now);
}

static void testHandshake() {
  LinkHealth health;
  health.tick(100000);  // ESP32 can wait indefinitely for an unpowered Daisy.
  assert(!health.ready() && !health.receiving());
  assert(health.uartRecoveryNeeded(100000, 99990));
  acknowledge(health, 40, 100010, 20);
  assert(!health.ready() && health.receiving());
  assert(!health.uartRecoveryNeeded(100010, 100010));
  acknowledge(health, 41, 100050, 60);
  assert(!health.ready());
  acknowledge(health, 42, 100090, 100);
  assert(health.ready());
  // Receiving repeated status does not prove controls are still reaching Daisy.
  StatusPayload frozen = {};
  frozen.flags = STATUS_LINK_ACTIVE;
  frozen.acceptedPackets = 10;
  frozen.lastControlSequence = 42;
  for (uint32_t now = 100110; now < 100700; now += 20) {
    frozen.uptimeMs = now;
    health.status(frozen, now);
  }
  assert(!health.ready() && health.receiving());
  assert(!health.uartRecoveryNeeded(100690, 100680));
  assert(health.uartRecoveryNeeded(100690, 100000));
  acknowledge(health, 60, 100710, 100710);
  acknowledge(health, 61, 100750, 100750);
  acknowledge(health, 62, 100790, 100790);
  assert(health.ready());
  health.tick(101291);  // Return direction disconnected.
  assert(!health.ready() && !health.receiving());
  acknowledge(health, 80, 101310, 101310);
  acknowledge(health, 81, 101350, 101350);
  acknowledge(health, 82, 101390, 101390);
  assert(health.ready());
  acknowledge(health, 83, 101430, 10);  // Daisy reboots before timeout expires.
  assert(!health.ready());
  acknowledge(health, 84, 101470, 50);
  acknowledge(health, 85, 101510, 90);
  assert(health.ready());
  health.reset();  // ESP32 reboot while Daisy remains powered.
  health.status(frozen, 101520);
  assert(!health.ready());
  acknowledge(health, 400, 101550, 500000);
  acknowledge(health, 401, 101590, 500040);
  acknowledge(health, 402, 101630, 500080);
  assert(health.ready());
  frozen.flags = 0;
  frozen.uptimeMs = 500090;
  health.status(frozen, 101640);
  assert(!health.ready());
}

static void testWrapAndStaleFrames() {
  LinkHealth health;
  acknowledge(health, 65534, 0xFFFFFFD0U, 0xFFFFFFD0U);
  acknowledge(health, 65535, 0xFFFFFFF8U, 0xFFFFFFF8U);
  acknowledge(health, 0, 32, 32);
  assert(health.ready());
  health.tick(533);
  assert(!health.ready());
  health.reset();
  StatusPayload stale = {};
  stale.flags = STATUS_LINK_ACTIVE;
  stale.acceptedPackets = 10;
  for (uint16_t seq = 1; seq < 5; ++seq) {
    stale.lastControlSequence = seq;
    stale.uptimeMs = 1000 + seq;
    health.status(stale, 1000 + seq);  // Never sent by this boot.
  }
  assert(!health.ready());
  acknowledge(health, 100, 1100, 1100);
  for (unsigned i = 0; i < 10; ++i) acknowledge(health, 100, 1120 + i, 1120 + i);
  assert(!health.ready());  // Duplicates cannot finish the handshake.
  acknowledge(health, 101, 1160, 1160);
  acknowledge(health, 102, 1200, 1200);
  assert(health.ready());
}

template <typename Packet, typename Payload, uint8_t Type>
static void testParser() {
  Payload payload = {};
  Packet source;
  preparePacket<Packet, Payload>(source, Type, 1234, payload);
  const auto* bytes = reinterpret_cast<const uint8_t*>(&source);
  // Every possible partial-frame interruption followed by a complete frame.
  for (size_t cut = 0; cut < sizeof(Packet); ++cut) {
    StreamParser<Packet, Payload, Type> parser;
    Packet decoded;
    for (size_t i = 0; i < cut; ++i) assert(!parser.push(bytes[i], decoded));
    unsigned received = 0;
    for (size_t i = 0; i < sizeof(Packet); ++i) {
      if (parser.push(bytes[i], decoded)) ++received;
    }
    assert(received == 1 && decoded.header.sequence == 1234);
  }
  StreamParser<Packet, Payload, Type> parser;
  Packet decoded;
  auto corrupt = std::vector<uint8_t>(bytes, bytes + sizeof(Packet));
  corrupt[sizeof(Packet) - 1] ^= 0x80;
  for (auto byte : corrupt) assert(!parser.push(byte, decoded));
  assert(parser.rejected == 1);
  unsigned received = 0;
  for (auto byte : std::vector<uint8_t>(bytes, bytes + sizeof(Packet))) {
    if (parser.push(byte, decoded)) ++received;
  }
  assert(received == 1);
  parser.push(SYNC_0, decoded);
  parser.reset();  // UART recovery must discard any old partial header.
  received = 0;
  for (size_t i = 0; i < sizeof(Packet); ++i) {
    if (parser.push(bytes[i], decoded)) ++received;
  }
  assert(received == 1);
}

int main() {
  testHandshake();
  testWrapAndStaleFrames();
  testParser<ControlPacket, ControlPayload, CONTROL_PACKET_TYPE>();
  testParser<StatusPacket, StatusPayload, STATUS_PACKET_TYPE>();
  std::puts("PASS: delayed boot, either peer reboot, one-way loss, reconnect, stale ACKs, counter wrap, truncated/corrupt frames");
}
