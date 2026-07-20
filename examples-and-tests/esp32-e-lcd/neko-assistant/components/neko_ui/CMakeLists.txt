idf_component_register(
    SRCS "display_manager.cpp" "cat_avatar.cpp" "ui_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES neko_events neko_board neko_core u8g2_st7305 driver
)
