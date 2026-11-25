#include "ota_manager.hpp"

#include <cstdio>
#include <cstring>
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_check.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "event_bus.hpp"

namespace neko {

esp_err_t OtaManager::check_power() const {
    const BatteryStatus status = battery_.status();
    return status.valid && (status.percent >= 50 || status.charging_estimate)
               ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t OtaManager::confirm_running_image() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state{};
    if (esp_ota_get_state_partition(running, &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY) {
        return esp_ota_mark_app_valid_cancel_rollback();
    }
    return ESP_OK;
}

esp_err_t OtaManager::update_https(const char *url) {
    if (!url || std::strncmp(url, "https://", 8) != 0) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(check_power(), "ota", "battery must be >=50%% or charging");
    EventBus::instance().publish(make_event(EventType::OtaStarted));
    esp_http_client_config_t http{};
    http.url = url;
    http.timeout_ms = 15000;
    http.crt_bundle_attach = esp_crt_bundle_attach;
    http.keep_alive_enable = true;
    esp_https_ota_config_t config{};
    config.http_config = &http;
    const esp_err_t error = esp_https_ota(&config);
    EventBus::instance().publish(make_event(error == ESP_OK ? EventType::OtaFinished : EventType::Error,
                                            error == ESP_OK ? "OTA verificato; riavvio" : "OTA HTTPS fallito",
                                            error));
    if (error == ESP_OK) esp_restart();
    return error;
}

esp_err_t OtaManager::update_from_sd(const char *path) {
    if (!path) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(check_power(), "ota", "battery must be >=50%% or charging");
    FILE *file = std::fopen(path, "rb");
    if (!file) return ESP_ERR_NOT_FOUND;
    EventBus::instance().publish(make_event(EventType::OtaStarted));
    const esp_partition_t *partition = esp_ota_get_next_update_partition(nullptr);
    esp_ota_handle_t handle = 0;
    esp_err_t error = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &handle);
    uint8_t buffer[4096];
    size_t total = 0;
    while (error == ESP_OK) {
        const size_t count = std::fread(buffer, 1, sizeof(buffer), file);
        if (!count) break;
        error = esp_ota_write(handle, buffer, count);
        total += count;
        if ((total & 0xFFFFU) == 0) {
            EventBus::instance().publish(make_event(EventType::OtaProgress, nullptr,
                                                    static_cast<int32_t>(total)));
        }
    }
    std::fclose(file);
    if (error == ESP_OK) error = esp_ota_end(handle); else esp_ota_abort(handle);
    if (error == ESP_OK) error = esp_ota_set_boot_partition(partition);
    EventBus::instance().publish(make_event(error == ESP_OK ? EventType::OtaFinished : EventType::Error,
                                            error == ESP_OK ? "Pacchetto OTA verificato" : "OTA microSD fallito",
                                            error));
    if (error == ESP_OK) esp_restart();
    return error;
}

}  // namespace neko
