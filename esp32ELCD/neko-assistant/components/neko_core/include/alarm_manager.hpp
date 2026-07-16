#pragma once

#include <array>
#include <ctime>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

namespace neko {

struct Alarm {
    uint32_t id{0};
    uint8_t hour{7};
    uint8_t minute{0};
    uint8_t weekdays{0x7F};  // bit 0 Sunday
    bool enabled{false};
    char label[32]{};
};

struct Reminder {
    uint32_t id{0};
    time_t due_epoch{0};
    bool enabled{false};
    char text[64]{};
};

class AlarmManager final {
public:
    esp_err_t init();
    esp_err_t set_alarm(const Alarm &alarm);
    esp_err_t remove_alarm(uint32_t id);
    esp_err_t add_reminder(time_t due, const char *text, uint32_t *id = nullptr);
    esp_err_t snooze(uint32_t minutes = 9);
    std::array<Alarm, 8> alarms() const;
    std::array<Reminder, 8> reminders() const;
    time_t next_event() const;

private:
    struct Persisted { std::array<Alarm, 8> alarms; std::array<Reminder, 8> reminders; uint32_t next_id; };
    static void task_entry(void *context);
    void run();
    esp_err_t persist_locked();

    nvs_handle_t nvs_{0};
    TaskHandle_t task_{nullptr};
    mutable SemaphoreHandle_t mutex_{nullptr};
    Persisted data_{};
    time_t last_alarm_minute_{0};
};

}  // namespace neko
