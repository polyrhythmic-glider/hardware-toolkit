#include "app_controller.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "board_bus.hpp"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "event_bus.hpp"
#include "nvs_flash.h"

namespace neko {
RTC_NOINIT_ATTR static uint32_t g_crash_boot_count;

AppController::AppController()
    : time_(rtc_), wifi_(settings_),
      ble_(settings_, wifi_, battery_, sensor_, time_, timers_, alarms_, notifications_, audio_),
      ota_(battery_), ui_(display_), diagnostics_(BoardBus::instance(), display_) {}

bool AppController::detect_safe_mode() {
    const esp_reset_reason_t reason = esp_reset_reason();
    const bool crash = reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
                       reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT;
    g_crash_boot_count = crash ? g_crash_boot_count + 1 : 0;
    return g_crash_boot_count >= 3;
}

esp_err_t AppController::init() {
    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), "app", "NVS recovery erase failed");
        error = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(error, "app", "NVS init failed");
    ESP_RETURN_ON_ERROR(EventBus::instance().init(), "app", "event bus failed");
    safe_mode_ = detect_safe_mode();
    ota_.confirm_running_image();
    ESP_RETURN_ON_ERROR(settings_.init(), "app", "settings failed");
    const AppSettings settings = settings_.get();
    ESP_RETURN_ON_ERROR(BoardBus::instance().init(), "app", "I2C failed");
    rtc_.init();
    time_.init(settings.timezone);  // RTC absence is a recoverable condition.
    ESP_RETURN_ON_ERROR(power_.init(safe_mode_ ? PowerProfile::Saver : settings.power_profile),
                        "app", "power management failed");
    ESP_RETURN_ON_ERROR(display_.init(), "app", "display failed");
    cat_.init();
    update_model();
    render(true);

    input_.init();
    timers_.init();
    alarms_.init();
    battery_.init();
    sensor_.set_board_heat_offset(settings.sensor_heat_offset_c);
    sensor_.init(power_.policy().sensor_period_s);
    if (storage_.mount() != ESP_OK) ESP_LOGI("app", "microSD absent; continuing");
    if (!safe_mode_) {
        if (audio_.init() != ESP_OK) ESP_LOGW("app", "audio unavailable; continuing silently");
        else audio_.set_volume(settings.volume);
        if (wifi_.init() != ESP_OK) ESP_LOGW("app", "Wi-Fi unavailable");
        if (ble_.init(settings.cat_name) != 0) ESP_LOGW("app", "BLE unavailable");
        if (settings.wifi_configured) {
            xTaskCreatePinnedToCore(connectivity_task, "neko_connect", 4096, this, 3,
                                    &connectivity_task_, 0);
        }
    } else {
        notifications_.push("Safe mode: radio e audio disabilitati", NotificationPriority::High, 300);
    }
    diagnostics_.log_boot();
    esp_task_wdt_add(nullptr);
    EventBus::instance().publish(make_event(EventType::Boot));
    return ESP_OK;
}

void AppController::connectivity_task(void *context) {
    auto *self = static_cast<AppController *>(context);
    if (self->wifi_.connect(15'000) == ESP_OK) {
        const uint32_t window = self->power_.policy().wifi_window_s;
        if (window) {
            vTaskDelay(pdMS_TO_TICKS(window * 1000U));
            self->time_.stop_sntp();
            self->wifi_.set_enabled(false);
        }
    }
    self->connectivity_task_ = nullptr;
    vTaskDelete(nullptr);
}

void AppController::update_model() {
    model_.now = time_.now();
    model_.time = time_.status();
    model_.battery = battery_.status();
    model_.environment = sensor_.reading();
    storage_.refresh();
    model_.storage = storage_.status();
    model_.cat = cat_.presentation();
    const WifiStatus wifi = wifi_.status();
    model_.wifi_connected = wifi.connected;
    model_.wifi_rssi = wifi.rssi;
    std::strncpy(model_.ip, wifi.ip, sizeof(model_.ip) - 1);
    model_.ble_connected = ble_.connected();
    const AppSettings settings = settings_.get();
    std::strncpy(model_.cat_name, settings.cat_name, sizeof(model_.cat_name) - 1);
    model_.volume = settings.volume;
    model_.muted = audio_.status().muted;
    std::strncpy(model_.status_line, model_.cat.message, sizeof(model_.status_line) - 1);

    const time_t next_alarm = alarms_.next_event();
    time_t next_timer_epoch = 0;
    int64_t shortest_us = INT64_MAX;
    for (const auto &timer : timers_.snapshot()) if (timer.active && timer.remaining_us < shortest_us) {
        shortest_us = timer.remaining_us;
        next_timer_epoch = model_.now + timer.remaining_us / 1'000'000;
    }
    const time_t next = next_alarm && next_timer_epoch ? std::min(next_alarm, next_timer_epoch)
                                                       : std::max(next_alarm, next_timer_epoch);
    if (next) {
        tm local{}; localtime_r(&next, &local);
        std::strftime(model_.next_event, sizeof(model_.next_event), "PROSSIMO %a %H:%M", &local);
    } else {
        std::strncpy(model_.next_event, "NESSUN EVENTO", sizeof(model_.next_event) - 1);
    }
    diagnostics_.populate(model_);
    ui_.set_model(model_);
}

void AppController::render(bool full) {
    update_model();
    ui_.render(full);
}

void AppController::handle_event(const Event &event) {
    const bool cat_changed = cat_.handle(event);
    bool full_refresh = false;
    switch (event.type) {
    case EventType::InputShort:
        ui_.next_page(); full_refresh = true; audio_.play_tone(900, 45); break;
    case EventType::InputDouble:
        if (ui_.page() == Page::Home) {
            timers_.start(5 * 60, "Timer rapido 5 min");
            notifications_.push("Timer rapido: cinque minuti.");
        } else {
            ui_.set_page(Page::Home);
        }
        full_refresh = true; audio_.play_tone(1100, 70); break;
    case EventType::InputLong:
        ui_.set_page(Page::Settings);
        ble_.open_provisioning_window();
        notifications_.push("Provisioning BLE aperto per due minuti.");
        full_refresh = true; break;
    case EventType::InputVeryLong:
        ui_.set_page(Page::Diagnostics); full_refresh = true; break;
    case EventType::WifiConnected:
        time_.start_sntp(); full_refresh = true; break;
    case EventType::TimerFinished:
        audio_.play_tone(1200, 700); full_refresh = true; break;
    case EventType::AlarmTriggered:
        audio_.play_tone(880, 1200); full_refresh = true; break;
    case EventType::ReminderDue:
    case EventType::NotificationReceived:
    case EventType::BleConnected:
    case EventType::BleDisconnected:
    case EventType::SensorUpdated:
    case EventType::BatteryUpdated:
    case EventType::TimeSynchronized:
        full_refresh = true; ble_.notify_status(); break;
    case EventType::RequestRender:
        ui_.set_page(Page::Diagnostics); full_refresh = true; break;
    case EventType::OtaStarted:
        ui_.set_page(Page::FirmwareUpdate); full_refresh = true; break;
    case EventType::BatteryCritical:
        power_.apply(PowerProfile::Critical);
        wifi_.set_enabled(false);
        audio_.set_muted(true);
        render(true);
        vTaskDelay(pdMS_TO_TICKS(2500));
        display_.set_power_save(true);
        power_.critical_shutdown();
        break;
    default: full_refresh = cat_changed; break;
    }
    if (full_refresh || cat_changed) render(full_refresh);
}

[[noreturn]] void AppController::run() {
    while (true) {
        Event event{};
        if (EventBus::instance().receive(event, pdMS_TO_TICKS(1000))) handle_event(event);
        if (cat_.tick()) render(false);
        const time_t now = time_.now();
        tm local{}; localtime_r(&now, &local);
        const AppSettings settings = settings_.get();
        if (local.tm_min != last_rendered_minute_ || settings.show_seconds) {
            last_rendered_minute_ = local.tm_min;
            render(false);
        }
        if (esp_timer_get_time() > 60'000'000) g_crash_boot_count = 0;
        esp_task_wdt_reset();
    }
}

}  // namespace neko
