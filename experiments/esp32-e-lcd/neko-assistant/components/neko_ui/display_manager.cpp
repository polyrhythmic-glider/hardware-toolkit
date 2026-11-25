#include "display_manager.hpp"

#include <algorithm>
#include "board_config.hpp"
#include "esp_check.h"

namespace neko {

esp_err_t DisplayManager::init() {
    mutex_ = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(mutex_, ESP_ERR_NO_MEM, "display", "mutex failed");
    u8g2_st7305_config_t config = u8g2_st7305_default_config();
    config.mosi_io = board::kDisplayMosi;
    config.sclk_io = board::kDisplaySclk;
    config.dc_io = board::kDisplayDc;
    config.cs_io = board::kDisplayCs;
    config.reset_io = board::kDisplayReset;
    config.spi_host = board::kDisplaySpiHost;
    config.rotation = U8G2_R1;  // logical 400x300 landscape on the physical 300x400 panel
    config.tile_buf_height = U8G2_ST7305_TILE_BUF_FULL;
    config.prefer_psram = true;
    ESP_RETURN_ON_ERROR(u8g2_st7305_init(&device_, &config), "display", "ST7305 failed");
    canvas_ = u8g2_st7305_get_u8g2(&device_);
    u8g2_SetBitmapMode(canvas_, 1);
    u8g2_SetDrawColor(canvas_, 1);
    clear();
    flush();
    return ESP_OK;
}

bool DisplayManager::lock(TickType_t wait) { return mutex_ && xSemaphoreTake(mutex_, wait) == pdTRUE; }
void DisplayManager::unlock() { if (mutex_) xSemaphoreGive(mutex_); }
void DisplayManager::clear() { if (canvas_) u8g2_ClearBuffer(canvas_); }
void DisplayManager::flush() { if (canvas_) u8g2_SendBuffer(canvas_); }

void DisplayManager::flush_area(int x, int y, int width, int height) {
    if (!canvas_ || width <= 0 || height <= 0) return;
    const int tx = std::max(0, x / 8);
    const int ty = std::max(0, y / 8);
    const int tx2 = std::min(49, (x + width + 7) / 8);
    const int ty2 = std::min(37, (y + height + 7) / 8);
    u8g2_UpdateDisplayArea(canvas_, tx, ty, tx2 - tx, ty2 - ty);
}

void DisplayManager::set_power_save(bool enabled) {
    if (canvas_) u8g2_SetPowerSave(canvas_, enabled ? 1 : 0);
}

}  // namespace neko
