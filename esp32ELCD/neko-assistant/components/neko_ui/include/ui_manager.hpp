#pragma once

#include <ctime>
#include "battery_manager.hpp"
#include "cat_state_machine.hpp"
#include "display_manager.hpp"
#include "sensor_manager.hpp"
#include "storage_manager.hpp"
#include "time_service.hpp"

namespace neko {

enum class Page : uint8_t {
    Home, Clock, Environment, Timers, Alarms, Reminders, Battery, Connections,
    Audio, Settings, SystemInfo, Diagnostics, CatFullscreen, Night, FirmwareUpdate,
    Count
};

struct UiModel {
    time_t now{0};
    TimeStatus time{};
    BatteryStatus battery{};
    EnvironmentReading environment{};
    StorageStatus storage{};
    CatPresentation cat{};
    bool wifi_connected{false};
    bool ble_connected{false};
    int wifi_rssi{0};
    char ip[16]{};
    char cat_name[24]{"Neko"};
    char next_event[48]{"Nessun evento"};
    char status_line[96]{"Tutto sotto controllo."};
    uint32_t uptime_seconds{0};
    uint32_t free_heap{0};
    uint32_t free_psram{0};
    uint32_t i2c_errors{0};
    uint32_t spi_errors{0};
    uint8_t volume{45};
    bool muted{false};
};

class UiManager final {
public:
    explicit UiManager(DisplayManager &display) : display_(display) {}
    void set_model(const UiModel &model) { model_ = model; }
    void next_page();
    void previous_page();
    void set_page(Page page);
    Page page() const { return page_; }
    void render(bool full_refresh = false);

private:
    void draw_header(u8g2_t *u, const char *title);
    void draw_home(u8g2_t *u);
    void draw_page(u8g2_t *u);
    void draw_battery_icon(u8g2_t *u, int x, int y);
    void draw_radio_icons(u8g2_t *u, int x, int y);

    DisplayManager &display_;
    UiModel model_{};
    Page page_{Page::Home};
    Page rendered_page_{Page::Count};
    uint8_t animation_frame_{0};
};

}  // namespace neko
