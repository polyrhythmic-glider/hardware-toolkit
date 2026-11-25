#include "notification_manager.hpp"

#include <algorithm>
#include <cstring>
#include "esp_timer.h"
#include "event_bus.hpp"

namespace neko {
static uint32_t hash_text(const char *text) {
    uint32_t hash = 2166136261U;
    while (*text) { hash = (hash ^ static_cast<uint8_t>(*text++)) * 16777619U; }
    return hash;
}

esp_err_t NotificationManager::push(const char *text, NotificationPriority priority,
                                    uint32_t ttl_seconds) {
    if (!text || !*text || std::strlen(text) >= 96) return ESP_ERR_INVALID_ARG;
    const int64_t now = esp_timer_get_time();
    const uint32_t hash = hash_text(text);
    portENTER_CRITICAL(&lock_);
    if (hash == last_hash_ && now - last_hash_time_ < 300'000'000) {
        portEXIT_CRITICAL(&lock_);
        return ESP_ERR_INVALID_STATE;  // five-minute repetition cooldown
    }
    Notification *slot = nullptr;
    for (auto &item : queue_) if (!item.id) { slot = &item; break; }
    if (!slot) {
        slot = &*std::min_element(queue_.begin(), queue_.end(), [](const auto &a, const auto &b) {
            return static_cast<int>(a.priority) < static_cast<int>(b.priority);
        });
        if (static_cast<int>(slot->priority) > static_cast<int>(priority)) {
            portEXIT_CRITICAL(&lock_);
            return ESP_ERR_NO_MEM;
        }
    }
    *slot = {};
    slot->id = next_id_++;
    slot->priority = priority;
    slot->created_us = now;
    slot->ttl_seconds = std::clamp<uint32_t>(ttl_seconds, 5, 3600);
    std::strncpy(slot->text, text, sizeof(slot->text) - 1);
    last_hash_ = hash;
    last_hash_time_ = now;
    Event event = make_event(EventType::NotificationReceived, text,
                             static_cast<int32_t>(priority));
    portEXIT_CRITICAL(&lock_);
    EventBus::instance().publish(event);
    return ESP_OK;
}

void NotificationManager::expire() {
    const int64_t now = esp_timer_get_time();
    portENTER_CRITICAL(&lock_);
    for (auto &item : queue_) if (item.id && now - item.created_us >
        static_cast<int64_t>(item.ttl_seconds) * 1'000'000) item = {};
    portEXIT_CRITICAL(&lock_);
}

bool NotificationManager::current(Notification &out) const {
    const_cast<NotificationManager *>(this)->expire();
    portENTER_CRITICAL(&lock_);
    const Notification *best = nullptr;
    for (const auto &item : queue_) if (item.id && (!best ||
        static_cast<int>(item.priority) > static_cast<int>(best->priority) ||
        (item.priority == best->priority && item.created_us < best->created_us))) best = &item;
    if (best) out = *best;
    portEXIT_CRITICAL(&lock_);
    return best != nullptr;
}

void NotificationManager::dismiss(uint32_t id) {
    portENTER_CRITICAL(&lock_);
    for (auto &item : queue_) if (item.id == id) item = {};
    portEXIT_CRITICAL(&lock_);
}

}  // namespace neko
