#pragma once

#include <atomic>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace neko {

class BoardBus final {
public:
    static BoardBus &instance();
    esp_err_t init();
    esp_err_t add_device(uint8_t address, uint32_t speed_hz, i2c_master_dev_handle_t *out);
    bool lock(TickType_t wait = pdMS_TO_TICKS(100));
    void unlock();
    i2c_master_bus_handle_t handle() const { return bus_; }
    uint32_t error_count() const { return errors_.load(); }
    void note_error() { errors_.fetch_add(1); }

private:
    i2c_master_bus_handle_t bus_{nullptr};
    SemaphoreHandle_t mutex_{nullptr};
    std::atomic<uint32_t> errors_{0};
};

class I2cGuard final {
public:
    explicit I2cGuard(BoardBus &bus, TickType_t wait = pdMS_TO_TICKS(100))
        : bus_(bus), held_(bus.lock(wait)) {}
    ~I2cGuard() { if (held_) bus_.unlock(); }
    bool held() const { return held_; }
private:
    BoardBus &bus_;
    bool held_;
};

}  // namespace neko
