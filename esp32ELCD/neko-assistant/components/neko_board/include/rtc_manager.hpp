#pragma once

#include <ctime>
#include "driver/i2c_master.h"
#include "esp_err.h"

namespace neko {

struct RtcStatus {
    bool present{false};
    bool valid{false};
    time_t last_read{0};
    esp_err_t last_error{ESP_ERR_INVALID_STATE};
};

class RtcManager final {
public:
    esp_err_t init();
    esp_err_t read(time_t &utc);
    esp_err_t write(time_t utc);
    esp_err_t configure_alarm(const tm &local_time);
    RtcStatus status() const { return status_; }

private:
    esp_err_t read_regs(uint8_t reg, uint8_t *data, size_t size);
    esp_err_t write_regs(uint8_t reg, const uint8_t *data, size_t size);
    static uint8_t to_bcd(int value);
    static int from_bcd(uint8_t value);

    i2c_master_dev_handle_t device_{nullptr};
    RtcStatus status_{};
};

}  // namespace neko
