#include "battery_manager.hpp"

#include <algorithm>
#include <cmath>
#include "board_config.hpp"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "event_bus.hpp"
#include "neko_algorithms.hpp"

namespace neko {

esp_err_t BatteryManager::init() {
    adc_oneshot_unit_init_cfg_t unit{};
    unit.unit_id = board::kBatteryAdcUnit;
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit, &adc_), "battery", "ADC init failed");
    adc_oneshot_chan_cfg_t channel{};
    channel.atten = ADC_ATTEN_DB_12;
    channel.bitwidth = ADC_BITWIDTH_12;
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(adc_, board::kBatteryAdcChannel, &channel),
                        "battery", "ADC channel failed");

    adc_cali_curve_fitting_config_t cal{};
    cal.unit_id = board::kBatteryAdcUnit;
    cal.chan = board::kBatteryAdcChannel;
    cal.atten = ADC_ATTEN_DB_12;
    cal.bitwidth = ADC_BITWIDTH_12;
    if (adc_cali_create_scheme_curve_fitting(&cal, &calibration_) != ESP_OK) {
        calibration_ = nullptr;
        ESP_LOGW("battery", "eFuse ADC calibration unavailable; voltage is approximate");
    }
    BaseType_t ok = xTaskCreatePinnedToCore(task_entry, "neko_battery", 4096, this, 3,
                                            &task_, 0);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

BatteryStatus BatteryManager::status() const {
    portENTER_CRITICAL(&lock_);
    BatteryStatus copy = status_;
    portEXIT_CRITICAL(&lock_);
    return copy;
}

uint8_t BatteryManager::voltage_to_percent(float v) {
    return algorithms::battery_percent(v);
}

esp_err_t BatteryManager::sample(BatteryStatus &out) {
    constexpr int kSamples = 16;
    int64_t sum = 0;
    for (int i = 0; i < kSamples; ++i) {
        int raw = 0;
        ESP_RETURN_ON_ERROR(adc_oneshot_read(adc_, board::kBatteryAdcChannel, &raw),
                            "battery", "ADC read failed");
        sum += raw;
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    out.raw = static_cast<int>(sum / kSamples);
    if (calibration_) {
        ESP_RETURN_ON_ERROR(adc_cali_raw_to_voltage(calibration_, out.raw, &out.adc_mv),
                            "battery", "ADC calibration failed");
        out.calibrated = true;
    } else {
        out.adc_mv = out.raw * 3100 / 4095;
        out.calibrated = false;
    }
    const float measured = out.adc_mv * 0.001F * board::kBatteryDividerRatio;
    history_[history_index_] = measured;
    history_index_ = (history_index_ + 1) % history_.size();
    history_count_ = std::min(history_count_ + 1, history_.size());
    float total = 0;
    for (size_t i = 0; i < history_count_; ++i) total += history_[i];
    out.voltage_v = total / static_cast<float>(history_count_);
    out.percent = voltage_to_percent(out.voltage_v);
    out.valid = out.voltage_v > 2.5F && out.voltage_v < 4.6F;
    out.sampled_at_us = esp_timer_get_time();

    // The schematic does not route ETA6098 STAT/VBUS to the MCU. A sustained
    // positive slope is therefore only an estimate, never an authoritative flag.
    if (history_count_ == history_.size()) {
        const size_t oldest = history_index_;
        const size_t newest = (history_index_ + history_.size() - 1) % history_.size();
        out.charging_estimate = history_[newest] - history_[oldest] > 0.025F;
    }
    const BatteryBand previous = status_.band;
    if (out.voltage_v <= (previous == BatteryBand::Critical ? 3.32F : 3.25F)) {
        out.band = BatteryBand::Critical;
    } else if (out.voltage_v <= (previous == BatteryBand::Low ? 3.58F : 3.50F)) {
        out.band = BatteryBand::Low;
    } else if (out.voltage_v >= 4.14F) {
        out.band = BatteryBand::Full;
    } else {
        out.band = BatteryBand::Normal;
    }
    return out.valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

void BatteryManager::task_entry(void *context) {
    static_cast<BatteryManager *>(context)->run();
}

void BatteryManager::run() {
    BatteryBand last_band = BatteryBand::Normal;
    while (true) {
        BatteryStatus next = status();
        const esp_err_t error = sample(next);
        portENTER_CRITICAL(&lock_);
        status_ = next;
        portEXIT_CRITICAL(&lock_);
        EventBus::instance().publish(make_event(EventType::BatteryUpdated, nullptr,
                                                next.percent, next.voltage_v));
        if (error == ESP_OK && next.band != last_band) {
            if (next.band == BatteryBand::Low) {
                EventBus::instance().publish(make_event(EventType::BatteryLow));
            } else if (next.band == BatteryBand::Critical) {
                EventBus::instance().publish(make_event(EventType::BatteryCritical));
            }
            last_band = next.band;
        }
        vTaskDelay(pdMS_TO_TICKS(30'000));
    }
}

}  // namespace neko
