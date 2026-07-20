#pragma once

#include <cstdint>
#include "esp_codec_dev.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace neko {

struct AudioStatus {
    bool codec_ready{false};
    bool microphone_ready{false};
    bool muted{false};
    uint8_t volume{45};
    uint16_t mic_level{0};
    esp_err_t last_error{ESP_ERR_INVALID_STATE};
};

class AudioManager final {
public:
    esp_err_t init();
    esp_err_t play_tone(uint16_t frequency_hz = 880, uint16_t duration_ms = 90);
    esp_err_t set_volume(uint8_t percent);
    void set_muted(bool muted);
    esp_err_t capture_diagnostic(const char *path, uint32_t duration_ms = 2000);
    AudioStatus status() const { return status_; }

private:
    struct ToneRequest { uint16_t frequency; uint16_t duration_ms; };
    static void task_entry(void *context);
    void run();
    esp_err_t synthesize(const ToneRequest &request);

    esp_codec_dev_handle_t playback_{nullptr};
    esp_codec_dev_handle_t record_{nullptr};
    QueueHandle_t queue_{nullptr};
    TaskHandle_t task_{nullptr};
    AudioStatus status_{};
};

}  // namespace neko
