#pragma once

#include <array>
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace neko {

enum class BatteryBand : uint8_t { Critical, Low, Normal, Full };

struct BatteryStatus {
    int raw{0};
    int adc_mv{0};
    float voltage_v{0};
    uint8_t percent{0};
    BatteryBand band{BatteryBand::Normal};
    bool charging_estimate{false};
    bool calibrated{false};
    bool valid{false};
    int64_t sampled_at_us{0};
};

class BatteryManager final {
public:
    esp_err_t init();
    BatteryStatus status() const;

private:
    static void task_entry(void *context);
    void run();
    esp_err_t sample(BatteryStatus &out);
    static uint8_t voltage_to_percent(float volts);

    adc_oneshot_unit_handle_t adc_{nullptr};
    adc_cali_handle_t calibration_{nullptr};
    TaskHandle_t task_{nullptr};
    mutable portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
    BatteryStatus status_{};
    std::array<float, 12> history_{};
    size_t history_count_{0};
    size_t history_index_{0};
};

}  // namespace neko
