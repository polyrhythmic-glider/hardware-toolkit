#include "timer_manager.hpp"

#include <algorithm>
#include <cstring>
#include "esp_timer.h"
#include "event_bus.hpp"

namespace neko {

esp_err_t TimerManager::init() {
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) return ESP_ERR_NO_MEM;
    esp_err_t error = nvs_open("neko_sched", NVS_READWRITE, &nvs_);
    if (error != ESP_OK) return error;
    size_t size = sizeof(slots_);
    if (nvs_get_blob(nvs_, "timers", slots_.data(), &size) != ESP_OK || size != sizeof(slots_)) {
        slots_ = {};
    }
    // esp_timer deadlines do not survive reset; active timers are restored paused
    // with their last persisted remaining value instead of firing spuriously.
    for (auto &slot : slots_) {
        if (slot.active && !slot.paused) slot.paused = true;
        next_id_ = std::max(next_id_, slot.id + 1);
    }
    BaseType_t ok = xTaskCreatePinnedToCore(task_entry, "neko_timers", 4096, this, 4,
                                            &task_, 0);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

TimerSlot *TimerManager::find_locked(uint32_t id) {
    for (auto &slot : slots_) if (slot.active && slot.id == id) return &slot;
    return nullptr;
}

esp_err_t TimerManager::persist_locked() {
    esp_err_t error = nvs_set_blob(nvs_, "timers", slots_.data(), sizeof(slots_));
    return error == ESP_OK ? nvs_commit(nvs_) : error;
}

esp_err_t TimerManager::start(uint32_t seconds, const char *label, uint32_t *id) {
    if (seconds == 0 || seconds > 7U * 24U * 3600U) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    TimerSlot *slot = nullptr;
    for (auto &candidate : slots_) if (!candidate.active) { slot = &candidate; break; }
    if (!slot) { xSemaphoreGive(mutex_); return ESP_ERR_NO_MEM; }
    *slot = {};
    slot->id = next_id_++;
    slot->remaining_us = static_cast<int64_t>(seconds) * 1'000'000;
    slot->deadline_us = esp_timer_get_time() + slot->remaining_us;
    slot->active = true;
    if (label) std::strncpy(slot->label, label, sizeof(slot->label) - 1);
    if (id) *id = slot->id;
    esp_err_t error = persist_locked();
    xSemaphoreGive(mutex_);
    return error;
}

esp_err_t TimerManager::pause(uint32_t id) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    TimerSlot *slot = find_locked(id);
    if (!slot || slot->paused) { xSemaphoreGive(mutex_); return ESP_ERR_NOT_FOUND; }
    slot->remaining_us = std::max<int64_t>(0, slot->deadline_us - esp_timer_get_time());
    slot->paused = true;
    esp_err_t error = persist_locked();
    xSemaphoreGive(mutex_);
    return error;
}

esp_err_t TimerManager::resume(uint32_t id) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    TimerSlot *slot = find_locked(id);
    if (!slot || !slot->paused) { xSemaphoreGive(mutex_); return ESP_ERR_NOT_FOUND; }
    slot->deadline_us = esp_timer_get_time() + slot->remaining_us;
    slot->paused = false;
    esp_err_t error = persist_locked();
    xSemaphoreGive(mutex_);
    return error;
}

esp_err_t TimerManager::cancel(uint32_t id) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    TimerSlot *slot = find_locked(id);
    if (!slot) { xSemaphoreGive(mutex_); return ESP_ERR_NOT_FOUND; }
    *slot = {};
    esp_err_t error = persist_locked();
    xSemaphoreGive(mutex_);
    return error;
}

std::array<TimerSlot, 4> TimerManager::snapshot() const {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    auto copy = slots_;
    const int64_t now = esp_timer_get_time();
    for (auto &slot : copy) if (slot.active && !slot.paused) {
        slot.remaining_us = std::max<int64_t>(0, slot.deadline_us - now);
    }
    xSemaphoreGive(mutex_);
    return copy;
}

void TimerManager::task_entry(void *context) {
    static_cast<TimerManager *>(context)->run();
}

void TimerManager::run() {
    while (true) {
        const int64_t now = esp_timer_get_time();
        Event finished[4]{};
        size_t count = 0;
        xSemaphoreTake(mutex_, portMAX_DELAY);
        for (auto &slot : slots_) {
            if (slot.active && !slot.paused && now >= slot.deadline_us) {
                finished[count++] = make_event(EventType::TimerFinished, slot.label,
                                               static_cast<int32_t>(slot.id));
                slot = {};
            }
        }
        if (count) persist_locked();
        xSemaphoreGive(mutex_);
        for (size_t i = 0; i < count; ++i) EventBus::instance().publish(finished[i]);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

}  // namespace neko
