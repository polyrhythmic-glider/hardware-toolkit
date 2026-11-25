#pragma once

#include "battery_manager.hpp"
#include "esp_err.h"

namespace neko {

class OtaManager final {
public:
    explicit OtaManager(BatteryManager &battery) : battery_(battery) {}
    esp_err_t confirm_running_image();
    esp_err_t update_https(const char *url);
    esp_err_t update_from_sd(const char *path);
private:
    esp_err_t check_power() const;
    BatteryManager &battery_;
};

}  // namespace neko
