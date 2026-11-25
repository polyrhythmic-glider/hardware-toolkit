#pragma once
#include <cstdint>

// Debounced gestures run on the application task; no display or NVS work in callbacks.
class NekoButton {
public:
  enum class Event { None, Click, Double, Long, VeryLong };
  Event update(bool down, uint32_t now) {
    if (down != raw_) { raw_ = down; changed_ = now; }
    if (raw_ != down_ && now - changed_ >= 25) {
      down_ = raw_;
      if (down_) { pressed_ = now; held_ = false; }
      else if (!held_) {
        if (now - pressed_ >= 800) { pending_ = false; return Event::Long; }
        if (pending_ && now - released_ <= 300) { pending_ = false; return Event::Double; }
        pending_ = true; released_ = now;
      }
    }
    if (down_ && !held_ && now - pressed_ >= 5000) {
      held_ = true; pending_ = false; return Event::VeryLong;
    }
    if (!down_ && pending_ && now - released_ > 300) {
      pending_ = false; return Event::Click;
    }
    return Event::None;
  }
private:
  bool raw_ = false, down_ = false, held_ = false, pending_ = false;
  uint32_t changed_ = 0, pressed_ = 0, released_ = 0;
};

class NekoChord {
public:
  // Once both keys are involved, suppress their individual gestures until release.
  bool update(bool key, bool boot, uint32_t now) {
    if (key&&boot) {
      if (!tracking_) {tracking_=true;since_=now;}
      if (!fired_&&now-since_>=2000) {fired_=true;return true;}
    }
    if (tracking_&&!key&&!boot) {tracking_=false;fired_=false;}
    return false;
  }
  bool suppress() const {return tracking_;}
private:
  bool tracking_=false,fired_=false;
  uint32_t since_=0;
};
