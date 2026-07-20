#include "cat_state_machine.hpp"

#include <cstring>
#include "esp_timer.h"

namespace neko {

CatPresentation CatStateMachine::definition(CatState state) {
    switch (state) {
    case CatState::Greeting: return {state, CatExpression::Happy, "Buongiorno. Procediamo con stile.", 2, 5000, true, true};
    case CatState::Listening: return {state, CatExpression::Listening, "Ti ascolto.", 3, 15000, true, true};
    case CatState::Thinking: return {state, CatExpression::Thinking, "Un momento. Sto ordinando i pensieri.", 3, 15000, true, false};
    case CatState::Speaking: return {state, CatExpression::Focused, "Ecco il punto.", 3, 15000, true, false};
    case CatState::Notifying: return {state, CatExpression::Focused, "C'e una nota per te.", 4, 20000, true, true};
    case CatState::Sleepy: return {state, CatExpression::Sleepy, "Mantengo un occhio aperto. Uno basta.", 1, 0, true, false};
    case CatState::Charging: return {state, CatExpression::Happy, "La batteria apprezza questa presa.", 2, 10000, true, false};
    case CatState::LowBattery: return {state, CatExpression::LowBattery, "La batteria gradirebbe una presa.", 6, 0, true, true};
    case CatState::BatteryCritical: return {state, CatExpression::Worried, "Energia critica. Metto tutto al sicuro.", 10, 0, false, true};
    case CatState::Offline: return {state, CatExpression::Offline, "Offline. Resto comunque utile.", 3, 12000, true, false};
    case CatState::Error: return {state, CatExpression::Error, "Qualcosa non collabora. Controllo i dettagli.", 8, 0, true, true};
    case CatState::Alarm: return {state, CatExpression::Alarm, "Sveglia. L'eleganza non ammette ritardi.", 9, 0, false, true};
    case CatState::TimerDone: return {state, CatExpression::TimerDone, "Timer concluso. Puntuale, naturalmente.", 8, 0, false, true};
    case CatState::Hot: return {state, CatExpression::Hot, "Fa caldo. Il mio pelo protesta.", 4, 15000, true, false};
    case CatState::Humid: return {state, CatExpression::Humid, "Umidita alta. Atmosfera decisamente teatrale.", 4, 15000, true, false};
    case CatState::UpdateMode: return {state, CatExpression::Focused, "Aggiornamento in corso. Niente mosse brusche.", 10, 0, false, false};
    case CatState::BluetoothConnected: return {state, CatExpression::Bluetooth, "Bluetooth collegato. Ora possiamo coordinarci.", 3, 8000, true, false};
    case CatState::Idle:
    default: return {CatState::Idle, CatExpression::Neutral, "Tutto sotto controllo.", 0, 0, true, false};
    }
}

void CatStateMachine::init() { enter(CatState::Greeting); }

void CatStateMachine::enter(CatState state, const char *override_message) {
    presentation_ = definition(state);
    if (override_message && *override_message) {
        std::strncpy(message_, override_message, sizeof(message_) - 1);
        message_[sizeof(message_) - 1] = '\0';
        presentation_.message = message_;
    } else {
        message_[0] = '\0';
    }
    entered_us_ = esp_timer_get_time();
}

bool CatStateMachine::handle(const Event &event) {
    const int64_t now = esp_timer_get_time();
    CatState next = CatState::Idle;
    const char *message = nullptr;
    bool relevant = true;
    switch (event.type) {
    case EventType::BatteryLow: next = CatState::LowBattery; break;
    case EventType::BatteryCritical: next = CatState::BatteryCritical; break;
    case EventType::WifiDisconnected: next = CatState::Offline; break;
    case EventType::WifiConnected: next = CatState::Notifying; message = "La rete e tornata. Come previsto."; break;
    case EventType::BleConnected: next = CatState::BluetoothConnected; break;
    case EventType::TimerFinished: next = CatState::TimerDone; message = event.text; break;
    case EventType::AlarmTriggered: next = CatState::Alarm; message = event.text; break;
    case EventType::ReminderDue:
    case EventType::NotificationReceived: next = CatState::Notifying; message = event.text; break;
    case EventType::OtaStarted: next = CatState::UpdateMode; break;
    case EventType::Error: next = CatState::Error; message = event.text; break;
    case EventType::SensorUpdated:
        if (now < environment_cooldown_us_) return false;
        if (event.value_f >= 28.0F) next = CatState::Hot;
        else if (event.value >= 7000) next = CatState::Humid;
        else { relevant = false; }
        if (relevant) environment_cooldown_us_ = now + 30LL * 60LL * 1'000'000LL;
        break;
    default: relevant = false; break;
    }
    if (!relevant) return false;
    const CatPresentation candidate = definition(next);
    if (!presentation_.interruptible && candidate.priority <= presentation_.priority) return false;
    if (candidate.priority < presentation_.priority && presentation_.duration_ms == 0) return false;
    enter(next, message);
    return true;
}

bool CatStateMachine::tick() {
    if (presentation_.duration_ms && esp_timer_get_time() - entered_us_ >=
        static_cast<int64_t>(presentation_.duration_ms) * 1000) {
        enter(CatState::Idle);
        return true;
    }
    return false;
}

}  // namespace neko
