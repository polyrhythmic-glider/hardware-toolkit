#include "input_manager.hpp"

#include "board_config.hpp"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "event_bus.hpp"

namespace neko {

esp_err_t InputManager::init() {
    gpio_config_t config{};
    config.pin_bit_mask = 1ULL << board::kKey;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    ESP_RETURN_ON_ERROR(gpio_config(&config), "input", "KEY gpio config failed");
    BaseType_t ok = xTaskCreatePinnedToCore(task_entry, "neko_input", 3072, this, 5,
                                            &task_, 1);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

void InputManager::task_entry(void *context) {
    static_cast<InputManager *>(context)->run();
}

void InputManager::run() {
    constexpr int64_t kDebounceUs = 35'000;
    constexpr int64_t kDoubleUs = 350'000;
    constexpr int64_t kLongUs = 700'000;
    constexpr int64_t kVeryLongUs = 5'000'000;
    bool raw = false;
    bool stable = false;
    int64_t raw_changed = esp_timer_get_time();
    int64_t pressed_at = 0;
    int64_t pending_short_at = 0;

    while (true) {
        const int64_t now = esp_timer_get_time();
        const bool level = gpio_get_level(board::kKey) == 0;
        if (level != raw) {
            raw = level;
            raw_changed = now;
        }
        if (raw != stable && now - raw_changed >= kDebounceUs) {
            stable = raw;
            if (stable) {
                pressed_at = now;
            } else {
                const int64_t held = now - pressed_at;
                if (held >= kVeryLongUs) {
                    pending_short_at = 0;
                    EventBus::instance().publish(make_event(EventType::InputVeryLong));
                } else if (held >= kLongUs) {
                    pending_short_at = 0;
                    EventBus::instance().publish(make_event(EventType::InputLong));
                } else if (held >= kDebounceUs) {
                    if (pending_short_at && now - pending_short_at <= kDoubleUs) {
                        pending_short_at = 0;
                        EventBus::instance().publish(make_event(EventType::InputDouble));
                    } else {
                        pending_short_at = now;
                    }
                }
            }
        }
        if (pending_short_at && now - pending_short_at > kDoubleUs) {
            pending_short_at = 0;
            EventBus::instance().publish(make_event(EventType::InputShort));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

}  // namespace neko
