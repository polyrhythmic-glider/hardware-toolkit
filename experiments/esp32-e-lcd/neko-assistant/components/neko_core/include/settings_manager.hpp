#pragma once

#include <cstdint>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"

namespace neko {

enum class PowerProfile : uint8_t { Performance, Balanced, Saver, Night, Critical };
enum class CatPersonality : uint8_t { Refined, Dry, Quiet };

struct AppSettings {
    uint16_t schema_version{2};
    uint16_t size{0};
    uint32_t crc32{0};
    char cat_name[24]{"Neko"};
    char timezone[64]{"CET-1CEST,M3.5.0,M10.5.0/3"};
    char wifi_ssid[33]{};
    char wifi_password[65]{};
    PowerProfile power_profile{PowerProfile::Balanced};
    CatPersonality personality{CatPersonality::Refined};
    uint8_t volume{45};
    uint8_t wifi_configured{0};
    uint8_t ble_bonding_required{1};
    uint8_t show_seconds{0};
    float sensor_heat_offset_c{1.5F};
};

class SettingsManager final {
public:
    esp_err_t init();
    AppSettings get() const;
    esp_err_t update(const AppSettings &settings);
    esp_err_t factory_reset();

private:
    static uint32_t checksum(AppSettings value);
    static void defaults(AppSettings &value);
    esp_err_t load();
    esp_err_t save_locked();

    nvs_handle_t nvs_{0};
    mutable SemaphoreHandle_t mutex_{nullptr};
    AppSettings settings_{};
};

}  // namespace neko
