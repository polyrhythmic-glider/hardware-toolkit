#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "u8g2.h"
#include "u8g2_st7305.h"

namespace neko {

class DisplayManager final {
public:
    esp_err_t init();
    void clear();
    void flush();
    void flush_area(int x, int y, int width, int height);
    void set_power_save(bool enabled);
    bool lock(TickType_t wait = pdMS_TO_TICKS(100));
    void unlock();
    u8g2_t *canvas() { return canvas_; }
    uint32_t spi_errors() const { return spi_errors_; }

private:
    u8g2_st7305_t device_{};
    u8g2_t *canvas_{nullptr};
    SemaphoreHandle_t mutex_{nullptr};
    uint32_t spi_errors_{0};
};

class DisplayGuard final {
public:
    explicit DisplayGuard(DisplayManager &display) : display_(display), held_(display.lock()) {}
    ~DisplayGuard() { if (held_) display_.unlock(); }
    bool held() const { return held_; }
private:
    DisplayManager &display_;
    bool held_;
};

}  // namespace neko
