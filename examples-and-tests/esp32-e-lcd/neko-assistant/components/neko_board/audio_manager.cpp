#include "audio_manager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <vector>
#include "codec_board.h"
#include "codec_init.h"
#include "esp_log.h"

namespace neko {
static constexpr int kSampleRate = 16000;

esp_err_t AudioManager::init() {
    set_codec_board_type("S3_RLCD_4_2");
    codec_init_cfg_t config{};
    config.in_mode = CODEC_I2S_MODE_TDM;
    config.out_mode = CODEC_I2S_MODE_TDM;
    config.in_use_tdm = true;
    config.reuse_dev = false;
    if (init_codec(&config) != 0) {
        status_.last_error = ESP_FAIL;
        return ESP_FAIL;
    }
    playback_ = get_playback_handle();
    record_ = get_record_handle();
    status_.codec_ready = playback_ != nullptr;
    status_.microphone_ready = record_ != nullptr;
    status_.last_error = status_.codec_ready ? ESP_OK : ESP_FAIL;
    if (!status_.codec_ready) return ESP_FAIL;
    set_volume(status_.volume);
    queue_ = xQueueCreate(4, sizeof(ToneRequest));
    if (!queue_) return ESP_ERR_NO_MEM;
    BaseType_t ok = xTaskCreatePinnedToCore(task_entry, "neko_audio", 6144, this, 4,
                                            &task_, 0);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t AudioManager::set_volume(uint8_t percent) {
    status_.volume = std::min<uint8_t>(percent, 100);
    if (!playback_) return ESP_ERR_INVALID_STATE;
    return esp_codec_dev_set_out_vol(playback_, status_.muted ? 0 : status_.volume) == 0
               ? ESP_OK : ESP_FAIL;
}

void AudioManager::set_muted(bool muted) {
    status_.muted = muted;
    set_volume(status_.volume);
}

esp_err_t AudioManager::play_tone(uint16_t frequency_hz, uint16_t duration_ms) {
    if (!queue_ || status_.muted) return status_.muted ? ESP_OK : ESP_ERR_INVALID_STATE;
    ToneRequest request{frequency_hz, duration_ms};
    return xQueueSend(queue_, &request, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

void AudioManager::task_entry(void *context) {
    static_cast<AudioManager *>(context)->run();
}

esp_err_t AudioManager::synthesize(const ToneRequest &request) {
    esp_codec_dev_sample_info_t format{};
    format.sample_rate = kSampleRate;
    format.channel = 2;
    format.bits_per_sample = 16;
    if (esp_codec_dev_open(playback_, &format) != 0) return ESP_FAIL;
    const size_t frames = static_cast<size_t>(kSampleRate) * request.duration_ms / 1000;
    std::vector<int16_t> pcm(frames * 2);
    constexpr float pi = 3.14159265358979323846F;
    for (size_t i = 0; i < frames; ++i) {
        const float envelope = std::min(1.0F, static_cast<float>(i) / 160.0F) *
                               std::min(1.0F, static_cast<float>(frames - i) / 160.0F);
        const int16_t sample = static_cast<int16_t>(std::sin(2.0F * pi * request.frequency *
                                                     i / kSampleRate) * 5000 * envelope);
        pcm[i * 2] = sample;
        pcm[i * 2 + 1] = sample;
    }
    const int result = esp_codec_dev_write(playback_, pcm.data(), pcm.size() * sizeof(int16_t));
    esp_codec_dev_close(playback_);
    return result == 0 ? ESP_OK : ESP_FAIL;
}

void AudioManager::run() {
    ToneRequest request{};
    while (true) {
        if (xQueueReceive(queue_, &request, portMAX_DELAY) == pdTRUE) {
            status_.last_error = synthesize(request);
        }
    }
}

esp_err_t AudioManager::capture_diagnostic(const char *path, uint32_t duration_ms) {
    if (!record_ || !path || duration_ms > 5000) return ESP_ERR_INVALID_ARG;
    esp_codec_dev_sample_info_t format{};
    format.sample_rate = kSampleRate;
    format.channel = 2;
    format.bits_per_sample = 16;
    if (esp_codec_dev_open(record_, &format) != 0) return ESP_FAIL;
    FILE *file = std::fopen(path, "wb");
    if (!file) {
        esp_codec_dev_close(record_);
        return ESP_FAIL;
    }
    int16_t block[512]{};
    size_t remaining = static_cast<size_t>(kSampleRate) * duration_ms / 1000 * 2;
    uint32_t peak = 0;
    while (remaining) {
        const size_t samples = std::min(remaining, std::size(block));
        if (esp_codec_dev_read(record_, block, samples * sizeof(int16_t)) != 0) break;
        std::fwrite(block, sizeof(int16_t), samples, file);
        for (size_t i = 0; i < samples; ++i) peak = std::max(peak, static_cast<uint32_t>(std::abs(block[i])));
        remaining -= samples;
    }
    std::fclose(file);
    esp_codec_dev_close(record_);
    status_.mic_level = static_cast<uint16_t>(std::min<uint32_t>(peak, 32767));
    return remaining == 0 ? ESP_OK : ESP_FAIL;
}

}  // namespace neko
