#include "time_service.hpp"

#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "event_bus.hpp"

namespace neko {
TimeService *TimeService::active_ = nullptr;

esp_err_t TimeService::init(const char *posix_timezone) {
    active_ = this;
    setenv("TZ", posix_timezone && *posix_timezone ? posix_timezone
                                                    : "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    time_t rtc_time = 0;
    status_.rtc_error = rtc_.read(rtc_time);
    if (status_.rtc_error == ESP_OK) {
        timeval tv{rtc_time, 0};
        settimeofday(&tv, nullptr);
        status_.source = TimeSource::HardwareRtc;
        status_.valid = true;
        return ESP_OK;
    }
    status_.source = TimeSource::Unreliable;
    status_.valid = false;
    return status_.rtc_error;
}

void TimeService::start_sntp() {
    if (esp_sntp_enabled()) return;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, const_cast<char *>("pool.ntp.org"));
    esp_sntp_setservername(1, const_cast<char *>("time.cloudflare.com"));
    esp_sntp_set_time_sync_notification_cb(sync_callback);
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    esp_sntp_init();
}

void TimeService::stop_sntp() {
    if (esp_sntp_enabled()) esp_sntp_stop();
}

void TimeService::sync_callback(timeval *tv) {
    if (active_ && tv) active_->on_synchronized(tv->tv_sec);
}

void TimeService::on_synchronized(time_t utc) {
    status_.source = TimeSource::Network;
    status_.valid = true;
    status_.last_sntp_sync = utc;
    status_.rtc_error = rtc_.write(utc);
    EventBus::instance().publish(make_event(EventType::TimeSynchronized));
}

esp_err_t TimeService::set_time(time_t utc, TimeSource source) {
    if (utc < 1'672'531'200) return ESP_ERR_INVALID_ARG;  // 2023-01-01
    timeval tv{utc, 0};
    if (settimeofday(&tv, nullptr) != 0) return ESP_FAIL;
    status_.source = source;
    status_.valid = true;
    return rtc_.write(utc);
}

time_t TimeService::now() const {
    return std::time(nullptr);
}

}  // namespace neko
