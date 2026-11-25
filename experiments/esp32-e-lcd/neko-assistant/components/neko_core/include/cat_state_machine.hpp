#pragma once

#include <cstdint>
#include "event_bus.hpp"

namespace neko {

enum class CatState : uint8_t {
    Idle, Greeting, Listening, Thinking, Speaking, Notifying, Sleepy, Charging,
    LowBattery, Offline, Error, Alarm, TimerDone, Hot, Humid, UpdateMode,
    BluetoothConnected, BatteryCritical
};

enum class CatExpression : uint8_t {
    Neutral, Happy, Sleepy, Focused, Worried, LowBattery, Listening, Thinking,
    Error, Offline, Bluetooth, TimerDone, Alarm, Hot, Humid
};

struct CatPresentation {
    CatState state{CatState::Idle};
    CatExpression expression{CatExpression::Neutral};
    const char *message{"Tutto sotto controllo."};
    uint8_t priority{0};
    uint32_t duration_ms{0};
    bool interruptible{true};
    bool play_sound{false};
};

class CatStateMachine final {
public:
    void init();
    bool handle(const Event &event);
    bool tick();
    CatPresentation presentation() const { return presentation_; }

private:
    void enter(CatState state, const char *override_message = nullptr);
    static CatPresentation definition(CatState state);
    CatPresentation presentation_{};
    char message_[96]{};
    int64_t entered_us_{0};
    int64_t environment_cooldown_us_{0};
};

}  // namespace neko
