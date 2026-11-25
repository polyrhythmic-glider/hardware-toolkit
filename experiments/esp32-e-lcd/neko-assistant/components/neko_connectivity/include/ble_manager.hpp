#pragma once

#include <cstdint>
#include "alarm_manager.hpp"
#include "audio_manager.hpp"
#include "battery_manager.hpp"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "notification_manager.hpp"
#include "sensor_manager.hpp"
#include "settings_manager.hpp"
#include "time_service.hpp"
#include "timer_manager.hpp"
#include "wifi_manager.hpp"

namespace neko {

class BleManager final {
public:
    BleManager(SettingsManager &settings, WifiManager &wifi, BatteryManager &battery,
               SensorManager &sensor, TimeService &time, TimerManager &timers,
               AlarmManager &alarms, NotificationManager &notifications,
               AudioManager &audio)
        : settings_(settings), wifi_(wifi), battery_(battery), sensor_(sensor), time_(time),
          timers_(timers), alarms_(alarms), notifications_(notifications), audio_(audio) {}

    int init(const char *device_name);
    void open_provisioning_window(uint32_t seconds = 120);
    void notify_status();
    bool connected() const { return connection_handle_ != BLE_HS_CONN_HANDLE_NONE; }
    uint16_t connection_handle() const { return connection_handle_; }

private:
    enum CharacteristicId : uintptr_t {
        Battery, Temperature, Humidity, Model, Firmware, Protocol, Status, Time,
        Timezone, AlarmConfig, TimerConfig, NotificationText, CatName, WifiCredentials,
        Volume, Control, Diagnostics
    };

    static int gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                           ble_gatt_access_ctxt *ctxt, void *arg);
    static int gap_event(ble_gap_event *event, void *arg);
    static void on_sync();
    static void host_task(void *arg);
    int access(uint16_t conn_handle, ble_gatt_access_ctxt *ctxt, CharacteristicId id);
    int read_characteristic(ble_gatt_access_ctxt *ctxt, CharacteristicId id);
    int write_characteristic(uint16_t conn_handle, ble_gatt_access_ctxt *ctxt,
                             CharacteristicId id);
    bool encrypted(uint16_t conn_handle) const;
    void start_advertising();
    void build_gatt_table();

    static BleManager *active_;
    SettingsManager &settings_;
    WifiManager &wifi_;
    BatteryManager &battery_;
    SensorManager &sensor_;
    TimeService &time_;
    TimerManager &timers_;
    AlarmManager &alarms_;
    NotificationManager &notifications_;
    AudioManager &audio_;
    uint16_t connection_handle_{BLE_HS_CONN_HANDLE_NONE};
    uint16_t status_value_handle_{0};
    uint16_t battery_value_handle_{0};
    uint8_t own_address_type_{0};
    int64_t provisioning_until_us_{0};
};

}  // namespace neko
