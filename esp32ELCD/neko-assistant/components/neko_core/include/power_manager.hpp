#pragma once

#include <ctime>
#include "esp_err.h"
#include "settings_manager.hpp"

namespace neko {

struct PowerPolicy {
    uint32_t ui_refresh_ms;
    uint32_t sensor_period_s;
    uint32_t wifi_window_s;
    uint16_t ble_advertising_ms;
    bool audio_allowed;
    bool wifi_allowed;
    bool light_sleep_allowed;
};

class PowerManager final {
public:
    esp_err_t init(PowerProfile profile);
    esp_err_t apply(PowerProfile profile);
    PowerProfile profile() const { return profile_; }
    PowerPolicy policy() const;
    [[noreturn]] void deep_sleep_until(time_t wake_epoch);
    [[noreturn]] void critical_shutdown();

private:
    PowerProfile profile_{PowerProfile::Balanced};
};

}  // namespace neko
