#include "settings_manager.hpp"

#include <cstring>
#include "esp_check.h"
#include "esp_log.h"

namespace neko {
static constexpr uint16_t kCurrentSchema = 2;

void SettingsManager::defaults(AppSettings &value) {
    value = AppSettings{};
    value.schema_version = kCurrentSchema;
    value.size = sizeof(AppSettings);
}

uint32_t SettingsManager::checksum(AppSettings value) {
    value.crc32 = 0;
    const auto *bytes = reinterpret_cast<const uint8_t *>(&value);
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < sizeof(value); ++i) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

esp_err_t SettingsManager::init() {
    mutex_ = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(mutex_, ESP_ERR_NO_MEM, "settings", "mutex failed");
    esp_err_t error = nvs_open("neko_cfg", NVS_READWRITE, &nvs_);
    if (error != ESP_OK) return error;
    return load();
}

esp_err_t SettingsManager::load() {
    size_t size = sizeof(settings_);
    AppSettings loaded{};
    esp_err_t error = nvs_get_blob(nvs_, "settings", &loaded, &size);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        defaults(settings_);
        return save_locked();
    }
    if (error != ESP_OK || size != sizeof(AppSettings) || loaded.size != sizeof(AppSettings) ||
        loaded.schema_version != kCurrentSchema || checksum(loaded) != loaded.crc32) {
        ESP_LOGW("settings", "invalid/old schema; safe defaults restored");
        defaults(settings_);
        return save_locked();
    }
    settings_ = loaded;
    settings_.cat_name[sizeof(settings_.cat_name) - 1] = '\0';
    settings_.timezone[sizeof(settings_.timezone) - 1] = '\0';
    settings_.wifi_ssid[sizeof(settings_.wifi_ssid) - 1] = '\0';
    settings_.wifi_password[sizeof(settings_.wifi_password) - 1] = '\0';
    return ESP_OK;
}

AppSettings SettingsManager::get() const {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    AppSettings copy = settings_;
    xSemaphoreGive(mutex_);
    return copy;
}

esp_err_t SettingsManager::save_locked() {
    settings_.schema_version = kCurrentSchema;
    settings_.size = sizeof(AppSettings);
    settings_.crc32 = checksum(settings_);
    ESP_RETURN_ON_ERROR(nvs_set_blob(nvs_, "settings", &settings_, sizeof(settings_)),
                        "settings", "write failed");
    return nvs_commit(nvs_);
}

esp_err_t SettingsManager::update(const AppSettings &settings) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    settings_ = settings;
    settings_.cat_name[sizeof(settings_.cat_name) - 1] = '\0';
    settings_.timezone[sizeof(settings_.timezone) - 1] = '\0';
    settings_.wifi_ssid[sizeof(settings_.wifi_ssid) - 1] = '\0';
    settings_.wifi_password[sizeof(settings_.wifi_password) - 1] = '\0';
    esp_err_t error = save_locked();
    xSemaphoreGive(mutex_);
    return error;
}

esp_err_t SettingsManager::factory_reset() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    esp_err_t error = nvs_erase_all(nvs_);
    if (error == ESP_OK) error = nvs_commit(nvs_);
    defaults(settings_);
    if (error == ESP_OK) error = save_locked();
    xSemaphoreGive(mutex_);
    return error;
}

}  // namespace neko
