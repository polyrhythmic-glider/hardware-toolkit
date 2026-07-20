#include "board_bus.hpp"

#include "board_config.hpp"
#include "esp_check.h"
#include "esp_log.h"

namespace neko {
static constexpr char kTag[] = "board_bus";

BoardBus &BoardBus::instance() {
    static BoardBus instance;
    return instance;
}

esp_err_t BoardBus::init() {
    if (bus_) return ESP_OK;
    mutex_ = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(mutex_, ESP_ERR_NO_MEM, kTag, "mutex allocation failed");
    i2c_master_bus_config_t config{};
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = board::kI2cSda;
    config.scl_io_num = board::kI2cScl;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;
    return i2c_new_master_bus(&config, &bus_);
}

esp_err_t BoardBus::add_device(uint8_t address, uint32_t speed_hz,
                               i2c_master_dev_handle_t *out) {
    ESP_RETURN_ON_FALSE(bus_ && out, ESP_ERR_INVALID_STATE, kTag, "bus not ready");
    i2c_device_config_t config{};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address;
    config.scl_speed_hz = speed_hz;
    esp_err_t error = i2c_master_bus_add_device(bus_, &config, out);
    if (error != ESP_OK) note_error();
    return error;
}

bool BoardBus::lock(TickType_t wait) {
    return mutex_ && xSemaphoreTake(mutex_, wait) == pdTRUE;
}

void BoardBus::unlock() {
    if (mutex_) xSemaphoreGive(mutex_);
}

}  // namespace neko
