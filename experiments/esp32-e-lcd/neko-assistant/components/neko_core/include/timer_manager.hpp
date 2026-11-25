#pragma once

#include <array>
#include <cstdint>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

namespace neko {

struct TimerSlot {
    uint32_t id{0};
    char label[32]{};
    int64_t deadline_us{0};
    int64_t remaining_us{0};
    bool active{false};
    bool paused{false};
};

class TimerManager final {
public:
    esp_err_t init();
    esp_err_t start(uint32_t duration_seconds, const char *label, uint32_t *id = nullptr);
    esp_err_t pause(uint32_t id);
    esp_err_t resume(uint32_t id);
    esp_err_t cancel(uint32_t id);
    std::array<TimerSlot, 4> snapshot() const;

private:
    static void task_entry(void *context);
    void run();
    esp_err_t persist_locked();
    TimerSlot *find_locked(uint32_t id);

    nvs_handle_t nvs_{0};
    TaskHandle_t task_{nullptr};
    mutable SemaphoreHandle_t mutex_{nullptr};
    std::array<TimerSlot, 4> slots_{};
    uint32_t next_id_{1};
};

}  // namespace neko
