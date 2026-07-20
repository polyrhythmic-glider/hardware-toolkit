#include "event_bus.hpp"

#include <cstring>
#include "esp_timer.h"

namespace neko {

EventBus &EventBus::instance() {
    static EventBus bus;
    return bus;
}

esp_err_t EventBus::init(size_t depth) {
    if (queue_) return ESP_OK;
    queue_ = xQueueCreate(depth, sizeof(Event));
    return queue_ ? ESP_OK : ESP_ERR_NO_MEM;
}

bool EventBus::publish(const Event &event, TickType_t wait) {
    return queue_ && xQueueSend(queue_, &event, wait) == pdTRUE;
}

bool EventBus::receive(Event &event, TickType_t wait) {
    return queue_ && xQueueReceive(queue_, &event, wait) == pdTRUE;
}

Event make_event(EventType type, const char *text, int32_t value, float value_f) {
    Event event{};
    event.type = type;
    event.timestamp_us = esp_timer_get_time();
    event.value = value;
    event.value_f = value_f;
    if (text) {
        std::strncpy(event.text, text, sizeof(event.text) - 1);
    }
    return event;
}

}  // namespace neko
