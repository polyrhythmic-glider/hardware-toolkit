#pragma once

#include <ctime>
#include "esp_err.h"
#include "rtc_manager.hpp"

namespace neko {

enum class TimeSource : uint8_t { Unreliable, HardwareRtc, Network };

struct TimeStatus {
    TimeSource source{TimeSource::Unreliable};
    bool valid{false};
    time_t last_sntp_sync{0};
    esp_err_t rtc_error{ESP_ERR_INVALID_STATE};
};

class TimeService final {
public:
    explicit TimeService(RtcManager &rtc) : rtc_(rtc) {}
    esp_err_t init(const char *posix_timezone);
    void start_sntp();
    void stop_sntp();
    esp_err_t set_time(time_t utc, TimeSource source);
    time_t now() const;
    TimeStatus status() const { return status_; }

private:
    static void sync_callback(timeval *tv);
    void on_synchronized(time_t utc);
    static TimeService *active_;
    RtcManager &rtc_;
    TimeStatus status_{};
};

}  // namespace neko
