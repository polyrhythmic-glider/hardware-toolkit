idf_component_register(
    SRCS "app_main.cpp" "app_controller.cpp" "diagnostics.cpp"
    INCLUDE_DIRS "."
    REQUIRES neko_events neko_board neko_core neko_connectivity neko_ui
             nvs_flash esp_system esp_timer esp_task_wdt heap spiffs
)

spiffs_create_partition_image(assets ../assets FLASH_IN_PROJECT)
