idf_component_register(
    SRCS "wifi_manager.cpp" "ble_manager.cpp" "ota_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        neko_events neko_board neko_core
        esp_wifi esp_netif esp_event nvs_flash bt
        app_update esp_https_ota mbedtls fatfs
)
