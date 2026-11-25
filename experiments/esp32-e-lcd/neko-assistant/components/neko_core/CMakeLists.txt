idf_component_register(
    SRCS
        "settings_manager.cpp"
        "time_service.cpp"
        "power_manager.cpp"
        "timer_manager.cpp"
        "alarm_manager.cpp"
        "notification_manager.cpp"
        "cat_state_machine.cpp"
    INCLUDE_DIRS "include"
    REQUIRES neko_events neko_board nvs_flash esp_pm esp_timer lwip
)
