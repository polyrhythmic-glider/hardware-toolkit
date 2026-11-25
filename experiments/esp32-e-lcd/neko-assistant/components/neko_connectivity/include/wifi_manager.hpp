#pragma once

#include <cstdint>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "settings_manager.hpp"

namespace neko {

struct WifiStatus {
    bool initialized{false};
    bool connected{false};
    bool provisioning{false};
    int8_t rssi{0};
    char ip[16]{};
    uint32_t reconnects{0};
};

class WifiManager final {
public:
    explicit WifiManager(SettingsManager &settings) : settings_(settings) {}
    esp_err_t init();
    esp_err_t connect(uint32_t timeout_ms = 15000);
    void disconnect();
    esp_err_t update_credentials(const char *ssid, const char *password);
    esp_err_t scan(wifi_ap_record_t *records, uint16_t *count);
    void set_enabled(bool enabled);
    WifiStatus status() const { return status_; }

private:
    static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);
    void on_event(esp_event_base_t base, int32_t id, void *data);

    SettingsManager &settings_;
    EventGroupHandle_t events_{nullptr};
    esp_netif_t *netif_{nullptr};
    WifiStatus status_{};
    uint8_t retries_{0};
    bool enabled_{true};
};

}  // namespace neko
