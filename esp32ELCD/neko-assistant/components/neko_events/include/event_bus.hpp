#pragma once

#include <cstdint>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace neko {

enum class EventType : uint8_t {
    Boot,
    InputShort,
    InputDouble,
    InputLong,
    InputVeryLong,
    SensorUpdated,
    BatteryUpdated,
    BatteryLow,
    BatteryCritical,
    WifiConnecting,
    WifiConnected,
    WifiDisconnected,
    BleConnected,
    BleDisconnected,
    TimeSynchronized,
    TimerFinished,
    AlarmTriggered,
    ReminderDue,
    NotificationReceived,
    AudioLevel,
    StorageChanged,
    OtaStarted,
    OtaProgress,
    OtaFinished,
    Error,
    RequestRender,
};

struct Event {
    EventType type{EventType::Boot};
    int64_t timestamp_us{0};
    int32_t value{0};
    float value_f{0.0F};
    char text[96]{};
};

class EventBus final {
public:
    static EventBus &instance();
    esp_err_t init(size_t depth = 32);
    bool publish(const Event &event, TickType_t wait = 0);
    bool receive(Event &event, TickType_t wait = portMAX_DELAY);
    QueueHandle_t handle() const { return queue_; }

private:
    QueueHandle_t queue_{nullptr};
};

Event make_event(EventType type, const char *text = nullptr, int32_t value = 0,
                 float value_f = 0.0F);

}  // namespace neko
