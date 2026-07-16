#include "alarm_manager.hpp"

#include <algorithm>
#include <cstring>
#include "event_bus.hpp"

namespace neko {

esp_err_t AlarmManager::init() {
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) return ESP_ERR_NO_MEM;
    esp_err_t error = nvs_open("neko_sched", NVS_READWRITE, &nvs_);
    if (error != ESP_OK) return error;
    size_t size = sizeof(data_);
    if (nvs_get_blob(nvs_, "alarms", &data_, &size) != ESP_OK || size != sizeof(data_)) {
        data_ = {};
        data_.next_id = 1;
    }
    BaseType_t ok = xTaskCreatePinnedToCore(task_entry, "neko_alarms", 4096, this, 4,
                                            &task_, 0);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t AlarmManager::persist_locked() {
    esp_err_t error = nvs_set_blob(nvs_, "alarms", &data_, sizeof(data_));
    return error == ESP_OK ? nvs_commit(nvs_) : error;
}

esp_err_t AlarmManager::set_alarm(const Alarm &input) {
    if (input.hour > 23 || input.minute > 59 || input.weekdays == 0) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    Alarm *slot = nullptr;
    if (input.id) for (auto &alarm : data_.alarms) if (alarm.id == input.id) slot = &alarm;
    if (!slot) for (auto &alarm : data_.alarms) if (!alarm.id) { slot = &alarm; break; }
    if (!slot) { xSemaphoreGive(mutex_); return ESP_ERR_NO_MEM; }
    *slot = input;
    if (!slot->id) slot->id = data_.next_id++;
    slot->label[sizeof(slot->label) - 1] = '\0';
    esp_err_t error = persist_locked();
    xSemaphoreGive(mutex_);
    return error;
}

esp_err_t AlarmManager::remove_alarm(uint32_t id) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    for (auto &alarm : data_.alarms) if (alarm.id == id) {
        alarm = {};
        esp_err_t error = persist_locked();
        xSemaphoreGive(mutex_);
        return error;
    }
    xSemaphoreGive(mutex_);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t AlarmManager::add_reminder(time_t due, const char *text, uint32_t *id) {
    if (due <= std::time(nullptr) || !text || !*text) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    Reminder *slot = nullptr;
    for (auto &candidate : data_.reminders) if (!candidate.enabled) { slot = &candidate; break; }
    if (!slot) { xSemaphoreGive(mutex_); return ESP_ERR_NO_MEM; }
    *slot = {};
    slot->id = data_.next_id++;
    slot->due_epoch = due;
    slot->enabled = true;
    std::strncpy(slot->text, text, sizeof(slot->text) - 1);
    if (id) *id = slot->id;
    esp_err_t error = persist_locked();
    xSemaphoreGive(mutex_);
    return error;
}

esp_err_t AlarmManager::snooze(uint32_t minutes) {
    return add_reminder(std::time(nullptr) + std::clamp<uint32_t>(minutes, 1, 60) * 60,
                        "Sveglia rinviata");
}

std::array<Alarm, 8> AlarmManager::alarms() const {
    xSemaphoreTake(mutex_, portMAX_DELAY); auto copy = data_.alarms; xSemaphoreGive(mutex_); return copy;
}

std::array<Reminder, 8> AlarmManager::reminders() const {
    xSemaphoreTake(mutex_, portMAX_DELAY); auto copy = data_.reminders; xSemaphoreGive(mutex_); return copy;
}

time_t AlarmManager::next_event() const {
    const time_t now = std::time(nullptr);
    time_t next = 0;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    for (const auto &reminder : data_.reminders) if (reminder.enabled && reminder.due_epoch > now &&
        (!next || reminder.due_epoch < next)) next = reminder.due_epoch;
    // Search the next eight days for recurring alarms.
    for (int day = 0; day < 8; ++day) {
        time_t candidate_day = now + day * 86400;
        tm local{}; localtime_r(&candidate_day, &local);
        for (const auto &alarm : data_.alarms) if (alarm.enabled && (alarm.weekdays & (1U << local.tm_wday))) {
            local.tm_hour = alarm.hour; local.tm_min = alarm.minute; local.tm_sec = 0; local.tm_isdst = -1;
            time_t candidate = mktime(&local);
            if (candidate > now && (!next || candidate < next)) next = candidate;
        }
    }
    xSemaphoreGive(mutex_);
    return next;
}

void AlarmManager::task_entry(void *context) { static_cast<AlarmManager *>(context)->run(); }

void AlarmManager::run() {
    while (true) {
        const time_t now = std::time(nullptr);
        tm local{}; localtime_r(&now, &local);
        const time_t minute = now / 60;
        Event due[16]{}; size_t count = 0;
        xSemaphoreTake(mutex_, portMAX_DELAY);
        if (minute != last_alarm_minute_) {
            for (const auto &alarm : data_.alarms) {
                if (alarm.enabled && alarm.hour == local.tm_hour && alarm.minute == local.tm_min &&
                    (alarm.weekdays & (1U << local.tm_wday))) {
                    due[count++] = make_event(EventType::AlarmTriggered, alarm.label,
                                              static_cast<int32_t>(alarm.id));
                }
            }
            last_alarm_minute_ = minute;
        }
        bool changed = false;
        for (auto &reminder : data_.reminders) if (reminder.enabled && reminder.due_epoch <= now) {
            due[count++] = make_event(EventType::ReminderDue, reminder.text,
                                      static_cast<int32_t>(reminder.id));
            reminder.enabled = false;
            changed = true;
        }
        if (changed) persist_locked();
        xSemaphoreGive(mutex_);
        for (size_t i = 0; i < count; ++i) EventBus::instance().publish(due[i]);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

}  // namespace neko
