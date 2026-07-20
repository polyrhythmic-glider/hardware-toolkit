#pragma once

#include "alarm_manager.hpp"
#include "audio_manager.hpp"
#include "battery_manager.hpp"
#include "ble_manager.hpp"
#include "cat_state_machine.hpp"
#include "diagnostics.hpp"
#include "display_manager.hpp"
#include "input_manager.hpp"
#include "notification_manager.hpp"
#include "ota_manager.hpp"
#include "power_manager.hpp"
#include "rtc_manager.hpp"
#include "sensor_manager.hpp"
#include "settings_manager.hpp"
#include "storage_manager.hpp"
#include "time_service.hpp"
#include "timer_manager.hpp"
#include "ui_manager.hpp"
#include "wifi_manager.hpp"

namespace neko {

class AppController final {
public:
    AppController();
    esp_err_t init();
    [[noreturn]] void run();

private:
    static void connectivity_task(void *context);
    void handle_event(const Event &event);
    void update_model();
    void render(bool full = false);
    bool detect_safe_mode();

    SettingsManager settings_;
    RtcManager rtc_;
    TimeService time_;
    PowerManager power_;
    InputManager input_;
    BatteryManager battery_;
    SensorManager sensor_;
    StorageManager storage_;
    AudioManager audio_;
    TimerManager timers_;
    AlarmManager alarms_;
    NotificationManager notifications_;
    WifiManager wifi_;
    BleManager ble_;
    OtaManager ota_;
    DisplayManager display_;
    UiManager ui_;
    CatStateMachine cat_;
    Diagnostics diagnostics_;
    UiModel model_{};
    TaskHandle_t connectivity_task_{nullptr};
    bool safe_mode_{false};
    int last_rendered_minute_{-1};
};

}  // namespace neko
