#include "power_manager.hpp"

#include <algorithm>
#include "board_config.hpp"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"

namespace neko {

PowerPolicy PowerManager::policy() const {
    switch (profile_) {
    case PowerProfile::Performance: return {1000, 15, 0, 250, true, true, false};
    case PowerProfile::Balanced: return {30'000, 30, 120, 500, true, true, true};
    case PowerProfile::Saver: return {60'000, 120, 30, 1000, false, true, true};
    case PowerProfile::Night: return {300'000, 300, 15, 2000, false, false, true};
    case PowerProfile::Critical: return {600'000, 600, 0, 0, false, false, true};
    }
    return {30'000, 30, 120, 500, true, true, true};
}

esp_err_t PowerManager::init(PowerProfile profile) {
    return apply(profile);
}

esp_err_t PowerManager::apply(PowerProfile profile) {
    profile_ = profile;
    esp_pm_config_t config{};
    config.max_freq_mhz = profile == PowerProfile::Performance ? 240 : 160;
    config.min_freq_mhz = profile == PowerProfile::Critical ? 40 : 80;
    config.light_sleep_enable = policy().light_sleep_allowed;
    return esp_pm_configure(&config);
}

[[noreturn]] void PowerManager::deep_sleep_until(time_t wake_epoch) {
    const time_t current = std::time(nullptr);
    const uint64_t delay_us = static_cast<uint64_t>(std::max<time_t>(1, wake_epoch - current)) *
                              1'000'000ULL;
    esp_sleep_enable_timer_wakeup(delay_us);
    esp_sleep_enable_ext1_wakeup_io(1ULL << board::kKey, ESP_EXT1_WAKEUP_ANY_LOW);
    ESP_LOGI("power", "deep sleep, wake in %llu us", delay_us);
    esp_deep_sleep_start();
    __builtin_unreachable();
}

[[noreturn]] void PowerManager::critical_shutdown() {
    profile_ = PowerProfile::Critical;
    esp_sleep_enable_ext1_wakeup_io(1ULL << board::kKey, ESP_EXT1_WAKEUP_ANY_LOW);
    ESP_LOGW("power", "critical battery deep sleep; KEY wake enabled");
    esp_deep_sleep_start();
    __builtin_unreachable();
}

}  // namespace neko
