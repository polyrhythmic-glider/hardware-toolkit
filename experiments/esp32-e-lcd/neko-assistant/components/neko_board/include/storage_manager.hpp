#pragma once

#include <cstdint>
#include "esp_err.h"
#include "sdmmc_cmd.h"

namespace neko {

struct StorageStatus {
    bool mounted{false};
    uint64_t total_bytes{0};
    uint64_t free_bytes{0};
    esp_err_t last_error{ESP_ERR_INVALID_STATE};
};

class StorageManager final {
public:
    esp_err_t mount();
    esp_err_t unmount();
    esp_err_t refresh();
    esp_err_t append_log(const char *line);
    StorageStatus status() const { return status_; }

private:
    sdmmc_card_t *card_{nullptr};
    StorageStatus status_{};
};

}  // namespace neko
