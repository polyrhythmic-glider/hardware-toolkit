#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace neko {

struct EnvironmentReading {
    float temperature_c{0};
    float humidity_rh{0};
    bool valid{false};
    int64_t sampled_at_us{0};
    uint32_t errors{0};
};

class SensorManager final {
public:
    esp_err_t init(uint32_t interval_seconds = 30);
    EnvironmentReading reading() const;
    void set_board_heat_offset(float celsius) { heat_offset_c_ = celsius; }

private:
    static void task_entry(void *context);
    void run();
    esp_err_t sample(EnvironmentReading &result);
    static uint8_t crc8(const uint8_t *data, size_t size);

    i2c_master_dev_handle_t device_{nullptr};
    TaskHandle_t task_{nullptr};
    mutable portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
    EnvironmentReading reading_{};
    uint32_t interval_seconds_{30};
    float heat_offset_c_{1.5F};
};

}  // namespace neko
