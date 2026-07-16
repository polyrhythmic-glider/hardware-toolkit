#include "wifi_manager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include "esp_check.h"
#include "esp_log.h"
#include "event_bus.hpp"

namespace neko {
static constexpr EventBits_t kConnected = BIT0;
static constexpr EventBits_t kFailed = BIT1;

esp_err_t WifiManager::init() {
    events_ = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(events_, ESP_ERR_NO_MEM, "wifi", "event group failed");
    ESP_RETURN_ON_ERROR(esp_netif_init(), "wifi", "netif init failed");
    esp_err_t loop_error = esp_event_loop_create_default();
    if (loop_error != ESP_OK && loop_error != ESP_ERR_INVALID_STATE) return loop_error;
    netif_ = esp_netif_create_default_wifi_sta();
    ESP_RETURN_ON_FALSE(netif_, ESP_ERR_NO_MEM, "wifi", "STA netif failed");
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&config), "wifi", "driver init failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, this),
                        "wifi", "handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, this),
                        "wifi", "IP handler failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), "wifi", "storage failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), "wifi", "mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_ps(WIFI_PS_MIN_MODEM), "wifi", "power save failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), "wifi", "start failed");
    status_.initialized = true;
    return ESP_OK;
}

esp_err_t WifiManager::update_credentials(const char *ssid, const char *password) {
    if (!ssid || !*ssid || std::strlen(ssid) > 32 || !password || std::strlen(password) > 64) {
        return ESP_ERR_INVALID_ARG;
    }
    AppSettings value = settings_.get();
    std::strncpy(value.wifi_ssid, ssid, sizeof(value.wifi_ssid) - 1);
    std::strncpy(value.wifi_password, password, sizeof(value.wifi_password) - 1);
    value.wifi_configured = 1;
    return settings_.update(value);
}

esp_err_t WifiManager::connect(uint32_t timeout_ms) {
    if (!status_.initialized) return ESP_ERR_INVALID_STATE;
    const AppSettings value = settings_.get();
    if (!value.wifi_configured || !value.wifi_ssid[0]) return ESP_ERR_NOT_FOUND;
    enabled_ = true;
    wifi_config_t config{};
    std::memcpy(config.sta.ssid, value.wifi_ssid, std::strlen(value.wifi_ssid));
    std::memcpy(config.sta.password, value.wifi_password, std::strlen(value.wifi_password));
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    config.sta.failure_retry_cnt = 3;
    retries_ = 0;
    xEventGroupClearBits(events_, kConnected | kFailed);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), "wifi", "config failed");
    EventBus::instance().publish(make_event(EventType::WifiConnecting));
    ESP_RETURN_ON_ERROR(esp_wifi_connect(), "wifi", "connect failed");
    const EventBits_t bits = xEventGroupWaitBits(events_, kConnected | kFailed, pdTRUE, pdFALSE,
                                                 pdMS_TO_TICKS(timeout_ms));
    if (bits & kConnected) return ESP_OK;
    return bits & kFailed ? ESP_FAIL : ESP_ERR_TIMEOUT;
}

void WifiManager::disconnect() {
    esp_wifi_disconnect();
    status_.connected = false;
}

void WifiManager::set_enabled(bool enabled) {
    enabled_ = enabled;
    if (enabled) esp_wifi_start();
    else { disconnect(); esp_wifi_stop(); }
}

esp_err_t WifiManager::scan(wifi_ap_record_t *records, uint16_t *count) {
    if (!records || !count || *count == 0) return ESP_ERR_INVALID_ARG;
    wifi_scan_config_t scan{};
    scan.show_hidden = false;
    ESP_RETURN_ON_ERROR(esp_wifi_scan_start(&scan, true), "wifi", "scan failed");
    return esp_wifi_scan_get_ap_records(count, records);
}

void WifiManager::event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    static_cast<WifiManager *>(arg)->on_event(base, id, data);
}

void WifiManager::on_event(esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        status_.connected = false;
        status_.ip[0] = '\0';
        EventBus::instance().publish(make_event(EventType::WifiDisconnected));
        if (enabled_ && ++retries_ <= 5) {
            ++status_.reconnects;
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(events_, kFailed);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const auto *event = static_cast<ip_event_got_ip_t *>(data);
        std::snprintf(status_.ip, sizeof(status_.ip), IPSTR, IP2STR(&event->ip_info.ip));
        status_.connected = true;
        retries_ = 0;
        wifi_ap_record_t ap{};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) status_.rssi = ap.rssi;
        xEventGroupSetBits(events_, kConnected);
        EventBus::instance().publish(make_event(EventType::WifiConnected));
    }
}

}  // namespace neko
