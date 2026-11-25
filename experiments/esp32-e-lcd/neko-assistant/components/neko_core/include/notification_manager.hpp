#pragma once

#include <array>
#include <cstdint>
#include "esp_err.h"

namespace neko {

enum class NotificationPriority : uint8_t { Low, Normal, High, Critical };

struct Notification {
    uint32_t id{0};
    NotificationPriority priority{NotificationPriority::Normal};
    int64_t created_us{0};
    uint32_t ttl_seconds{30};
    char text[96]{};
};

class NotificationManager final {
public:
    esp_err_t push(const char *text, NotificationPriority priority = NotificationPriority::Normal,
                   uint32_t ttl_seconds = 30);
    bool current(Notification &out) const;
    void dismiss(uint32_t id);
    void expire();

private:
    mutable portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
    std::array<Notification, 8> queue_{};
    uint32_t next_id_{1};
    uint32_t last_hash_{0};
    int64_t last_hash_time_{0};
};

}  // namespace neko
