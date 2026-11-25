#include "diagnostics.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

namespace neko {

void Diagnostics::populate(UiModel &model) const {
    model.uptime_seconds = static_cast<uint32_t>(esp_timer_get_time() / 1'000'000);
    model.free_heap = esp_get_free_heap_size();
    model.free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    model.i2c_errors = bus_.error_count();
    model.spi_errors = display_.spi_errors();
}

void Diagnostics::log_boot() const {
    ESP_LOGI("diag", "reset=%d heap=%lu psram=%lu",
             static_cast<int>(esp_reset_reason()),
             static_cast<unsigned long>(esp_get_free_heap_size()),
             static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
}

}  // namespace neko
