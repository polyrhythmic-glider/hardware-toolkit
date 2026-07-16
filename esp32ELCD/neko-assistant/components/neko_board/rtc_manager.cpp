#include "rtc_manager.hpp"

#include <cstring>
#include "board_bus.hpp"
#include "board_config.hpp"
#include "esp_check.h"
#include "esp_log.h"
#include "neko_algorithms.hpp"

namespace neko {
static constexpr char kTag[] = "rtc";
static constexpr uint8_t kSecondsRegister = 0x04;

uint8_t RtcManager::to_bcd(int value) {
    return algorithms::to_bcd(value);
}

int RtcManager::from_bcd(uint8_t value) {
    return algorithms::from_bcd(value);
}

esp_err_t RtcManager::init() {
    esp_err_t error = BoardBus::instance().add_device(board::kRtcAddress, 100000, &device_);
    status_.present = error == ESP_OK;
    status_.last_error = error;
    return error;
}

esp_err_t RtcManager::read_regs(uint8_t reg, uint8_t *data, size_t size) {
    I2cGuard guard(BoardBus::instance());
    if (!guard.held()) return ESP_ERR_TIMEOUT;
    esp_err_t error = i2c_master_transmit_receive(device_, &reg, 1, data, size, 100);
    if (error != ESP_OK) BoardBus::instance().note_error();
    status_.last_error = error;
    return error;
}

esp_err_t RtcManager::write_regs(uint8_t reg, const uint8_t *data, size_t size) {
    uint8_t buffer[16]{};
    if (size + 1 > sizeof(buffer)) return ESP_ERR_INVALID_SIZE;
    buffer[0] = reg;
    std::memcpy(buffer + 1, data, size);
    I2cGuard guard(BoardBus::instance());
    if (!guard.held()) return ESP_ERR_TIMEOUT;
    esp_err_t error = i2c_master_transmit(device_, buffer, size + 1, 100);
    if (error != ESP_OK) BoardBus::instance().note_error();
    status_.last_error = error;
    return error;
}

esp_err_t RtcManager::read(time_t &utc) {
    uint8_t value[7]{};
    ESP_RETURN_ON_ERROR(read_regs(kSecondsRegister, value, sizeof(value)), kTag,
                        "PCF85063 read failed");
    status_.valid = (value[0] & 0x80U) == 0;
    tm calendar{};
    calendar.tm_sec = from_bcd(value[0] & 0x7F);
    calendar.tm_min = from_bcd(value[1] & 0x7F);
    calendar.tm_hour = from_bcd(value[2] & 0x3F);
    calendar.tm_mday = from_bcd(value[3] & 0x3F);
    calendar.tm_wday = from_bcd(value[4] & 0x07);
    calendar.tm_mon = from_bcd(value[5] & 0x1F) - 1;
    calendar.tm_year = from_bcd(value[6]) + 100;
    if (calendar.tm_year < 123 || calendar.tm_year > 199 || calendar.tm_mon < 0 ||
        calendar.tm_mon > 11 || calendar.tm_mday < 1 || calendar.tm_mday > 31 ||
        calendar.tm_hour > 23 || calendar.tm_min > 59 || calendar.tm_sec > 59) {
        status_.valid = false;
        return ESP_ERR_INVALID_RESPONSE;
    }
    // RTC stores local civil time. mktime applies the configured POSIX timezone.
    calendar.tm_isdst = -1;
    utc = mktime(&calendar);
    status_.last_read = utc;
    return status_.valid ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t RtcManager::write(time_t utc) {
    tm local{};
    localtime_r(&utc, &local);
    const uint8_t value[7] = {
        to_bcd(local.tm_sec), to_bcd(local.tm_min), to_bcd(local.tm_hour),
        to_bcd(local.tm_mday), to_bcd(local.tm_wday), to_bcd(local.tm_mon + 1),
        to_bcd((local.tm_year + 1900) % 100),
    };
    esp_err_t error = write_regs(kSecondsRegister, value, sizeof(value));
    if (error == ESP_OK) {
        status_.valid = true;
        status_.last_read = utc;
    }
    return error;
}

esp_err_t RtcManager::configure_alarm(const tm &local_time) {
    // PCF85063 alarm registers: second, minute, hour, day, weekday. Bit 7
    // disables a field. We match minute/hour/day and ignore seconds/weekday.
    const uint8_t alarm[5] = {
        0x80, to_bcd(local_time.tm_min), to_bcd(local_time.tm_hour),
        to_bcd(local_time.tm_mday), 0x80,
    };
    ESP_RETURN_ON_ERROR(write_regs(0x0B, alarm, sizeof(alarm)), kTag, "alarm write failed");
    uint8_t control2 = 0;
    ESP_RETURN_ON_ERROR(read_regs(0x01, &control2, 1), kTag, "control read failed");
    control2 = static_cast<uint8_t>((control2 & ~0x40U) | 0x80U);  // clear AF, set AIE
    return write_regs(0x01, &control2, 1);
}

}  // namespace neko
