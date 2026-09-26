#pragma once

#include "DaisyLinkProtocol.h"

namespace DaisyLinkProtocol {

// Both directions must work: status traffic alone is not a handshake.
// Match acknowledgements to snapshots actually queued by this ESP32 boot.
class LinkHealth {
 public:
  static constexpr uint32_t TIMEOUT_MS = 500;
  void reset() { *this = LinkHealth(); }

  void sent(uint16_t sequence, uint32_t now) {
    sent_[next_] = {sequence, now, true};
    next_ = (next_ + 1) % 16;
  }

  void status(const StatusPayload& payload, uint32_t now) {
    tick(now);
    if (hasStatus_ && static_cast<int32_t>(payload.uptimeMs - uptime_) < 0) {
      clearProof();  // Daisy rebooted, rather than its millis counter wrapping.
    }
    hasStatus_ = true;
    lastStatusMs_ = now;
    uptime_ = payload.uptimeMs;
    if (!(payload.flags & STATUS_LINK_ACTIVE) || payload.acceptedPackets == 0) {
      clearProof();
      return;
    }
    bool known = false;
    for (const auto& entry : sent_) {
      if (entry.valid && entry.sequence == payload.lastControlSequence &&
          now - entry.time <= TIMEOUT_MS) known = true;
    }
    if (!known) return;
    if (hasAck_ && static_cast<int16_t>(payload.lastControlSequence - ack_) <= 0) return;
    hasAck_ = true;
    ack_ = payload.lastControlSequence;
    lastAckMs_ = now;
    if (proof_ < 3) ++proof_;
  }

  void tick(uint32_t now) {
    if (hasStatus_ && now - lastStatusMs_ > TIMEOUT_MS) {
      hasStatus_ = false;
      clearProof();
    }
    if (hasAck_ && now - lastAckMs_ > TIMEOUT_MS) clearProof();
  }

  bool ready() const { return hasStatus_ && proof_ >= 3; }
  bool receiving() const { return hasStatus_; }
  bool uartRecoveryNeeded(uint32_t now, uint32_t lastQueuedMs) const {
    // An incomplete handshake is not a failed UART. Keep listening while
    // status is arriving and our outgoing queue continues to make progress.
    return !ready() && (!receiving() || now - lastQueuedMs > TIMEOUT_MS);
  }

 private:
  void clearProof() { proof_ = 0; hasAck_ = false; }
  struct Sent { uint16_t sequence; uint32_t time; bool valid; };
  Sent sent_[16] = {};
  unsigned next_ = 0;
  unsigned proof_ = 0;
  bool hasStatus_ = false;
  bool hasAck_ = false;
  uint16_t ack_ = 0;
  uint32_t lastStatusMs_ = 0, lastAckMs_ = 0, uptime_ = 0;
};

// Preserve a possible following frame after noise, a truncated transmission,
// or power-up in the middle of a packet. No allocation, suitable for DMA RX.
template <typename Packet, typename Payload, uint8_t Type>
class StreamParser {
 public:
  void reset() { index_ = 0; }
  bool push(uint8_t byte, Packet& packet) {
    if (index_ == 0 && byte != SYNC_0) return false;
    if (index_ == 1 && byte != SYNC_1) {
      index_ = byte == SYNC_0 ? 1U : 0U;
      buffer_[0] = SYNC_0;
      return false;
    }
    buffer_[index_++] = byte;
    if (index_ != sizeof(Packet)) return false;
    memcpy(&packet, buffer_, sizeof(packet));
    if (isPacketValid<Packet, Payload>(packet, Type)) {
      reset();
      return true;
    }
    ++rejected;
    for (size_t start = 1; start + 1 < sizeof(Packet); ++start) {
      if (buffer_[start] != SYNC_0 || buffer_[start + 1] != SYNC_1) continue;
      index_ = sizeof(Packet) - start;
      memmove(buffer_, buffer_ + start, index_);
      return false;
    }
    index_ = buffer_[sizeof(Packet) - 1] == SYNC_0 ? 1 : 0;
    if (index_) buffer_[0] = SYNC_0;
    return false;
  }
  uint32_t rejected = 0;

 private:
  uint8_t buffer_[sizeof(Packet)] = {};
  size_t index_ = 0;
};

}  // namespace DaisyLinkProtocol
