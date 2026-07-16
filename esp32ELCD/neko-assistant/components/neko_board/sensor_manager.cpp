#include "sensor_manager.hpp"

#include "board_bus.hpp"
#include "board_config.hpp"
#include "esp_check.h"
#include "esp_timer.h"
#include "event_bus.hpp"
#include "neko_algorithms.hpp"

namespace neko {

uint8_t SensorManager::crc8(const uint8_t *data, size_t size) {
    return algorithms::shtc3_crc8(data, size);
}

esp_err_t SensorManager::init(uint32_t interval_seconds) {
    interval_seconds_ = interval_seconds < 5 ? 5 : interval_seconds;
    ESP_RETURN_ON_ERROR(BoardBus::instance().add_device(board::kShtc3Address, 400000,
                                                         &device_), "shtc3", "add failed");
    BaseType_t ok = xTaskCreatePinnedToCore(task_entry, "neko_sensor", 4096, this, 3,
                                            &task_, 0);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

EnvironmentReading SensorManager::reading() const {
    portENTER_CRITICAL(&lock_);
    EnvironmentReading copy = reading_;
    portEXIT_CRITICAL(&lock_);
    return copy;
}

esp_err_t SensorManager::sample(EnvironmentReading &result) {
    const uint8_t wake[] = {0x35, 0x17};
    const uint8_t measure[] = {0x78, 0x66};
    const uint8_t sleep[] = {0xB0, 0x98};
    uint8_t data[6]{};
    I2cGuard guard(BoardBus::instance(), pdMS_TO_TICKS(250));
    if (!guard.held()) return ESP_ERR_TIMEOUT;
    esp_err_t error = i2c_master_transmit(device_, wake, sizeof(wake), 100);
    if (error == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(1));
        error = i2c_master_transmit(device_, measure, sizeof(measure), 100);
    }
    if (error == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(20));
        error = i2c_master_receive(device_, data, sizeof(data), 100);
    }
    i2c_master_transmit(device_, sleep, sizeof(sleep), 100);
    if (error != ESP_OK) {
        BoardBus::instance().note_error();
        return error;
    }
    if (crc8(data, 2) != data[2] || crc8(data + 3, 2) != data[5]) {
        return ESP_ERR_INVALID_CRC;
    }
    const uint16_t raw_t = static_cast<uint16_t>((data[0] << 8) | data[1]);
    const uint16_t raw_h = static_cast<uint16_t>((data[3] << 8) | data[4]);
    result.temperature_c = -45.0F + 175.0F * raw_t / 65536.0F - heat_offset_c_;
    result.humidity_rh = 100.0F * raw_h / 65536.0F;
    result.valid = result.temperature_c > -20.0F && result.temperature_c < 70.0F &&
                   result.humidity_rh >= 0.0F && result.humidity_rh <= 100.0F;
    result.sampled_at_us = esp_timer_get_time();
    return result.valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

void SensorManager::task_entry(void *context) {
    static_cast<SensorManager *>(context)->run();
}

void SensorManager::run() {
    while (true) {
        EnvironmentReading next = reading();
        esp_err_t error = sample(next);
        if (error != ESP_OK) {
            next.valid = false;
            ++next.errors;
        }
        portENTER_CRITICAL(&lock_);
        reading_ = next;
        portEXIT_CRITICAL(&lock_);
        EventBus::instance().publish(make_event(EventType::SensorUpdated, nullptr,
                                                next.valid ? static_cast<int32_t>(next.humidity_rh * 100) : -1,
                                                next.temperature_c));
        vTaskDelay(pdMS_TO_TICKS(interval_seconds_ * 1000U));
    }
}

}  // namespace neko
