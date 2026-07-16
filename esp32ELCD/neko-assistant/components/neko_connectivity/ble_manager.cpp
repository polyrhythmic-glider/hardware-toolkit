#include "ble_manager.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "event_bus.hpp"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "os/os_mbuf.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "store/config/ble_store_config.h"
#include "store/ble_store_util.h"

namespace neko {
BleManager *BleManager::active_ = nullptr;

static ble_gatt_chr_def g_battery_chars[2]{};
static ble_gatt_chr_def g_environment_chars[3]{};
static ble_gatt_chr_def g_device_chars[3]{};
static ble_gatt_chr_def g_custom_chars[13]{};
static ble_gatt_svc_def g_services[5]{};
static std::array<ble_uuid16_t, 8> g_standard_uuids{};
static std::array<ble_uuid16_t, 3> g_standard_service_uuids{};

static const ble_uuid128_t kCustomService = BLE_UUID128_INIT(
    0x7a, 0x2b, 0x6e, 0x10, 0x80, 0x52, 0x4d, 0x6f, 0x9e, 0xa1, 0x4e, 0x45, 0x4b, 0x4f, 0x00, 0x01);

static ble_uuid128_t custom_uuid(uint8_t id) {
    ble_uuid128_t uuid = kCustomService;
    uuid.value[15] = id;
    return uuid;
}

static std::array<ble_uuid128_t, 12> g_custom_uuids{};

static int append(ble_gatt_access_ctxt *ctxt, const void *data, size_t size) {
    return os_mbuf_append(ctxt->om, data, size) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int flatten(ble_gatt_access_ctxt *ctxt, uint8_t *data, size_t capacity, uint16_t &size) {
    const uint16_t packet = OS_MBUF_PKTLEN(ctxt->om);
    if (packet > capacity) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    if (ble_hs_mbuf_to_flat(ctxt->om, data, capacity, &size) != 0) return BLE_ATT_ERR_UNLIKELY;
    return 0;
}

void BleManager::build_gatt_table() {
    size_t standard_uuid_index = 0;
    auto chr16 = [&](ble_gatt_chr_def &chr, uint16_t uuid, CharacteristicId id, uint16_t flags,
                    uint16_t *handle = nullptr) {
        auto &uuid_value = g_standard_uuids[standard_uuid_index++];
        uuid_value.u.type = BLE_UUID_TYPE_16;
        uuid_value.value = uuid;
        chr.uuid = &uuid_value.u;
        chr.access_cb = BleManager::gatt_access;
        chr.arg = reinterpret_cast<void *>(static_cast<uintptr_t>(id));
        chr.flags = flags;
        chr.val_handle = handle;
    };
    chr16(g_battery_chars[0], 0x2A19, Battery, BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          &battery_value_handle_);
    chr16(g_environment_chars[0], 0x2A6E, Temperature, BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY);
    chr16(g_environment_chars[1], 0x2A6F, Humidity, BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY);
    chr16(g_device_chars[0], 0x2A24, Model, BLE_GATT_CHR_F_READ);
    chr16(g_device_chars[1], 0x2A26, Firmware, BLE_GATT_CHR_F_READ);

    const CharacteristicId ids[] = {Protocol, Status, Time, Timezone, AlarmConfig, TimerConfig,
        NotificationText, CatName, WifiCredentials, Volume, Control, Diagnostics};
    const uint16_t flags[] = {
        BLE_GATT_CHR_F_READ,
        static_cast<uint16_t>(BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY),
        static_cast<uint16_t>(BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        static_cast<uint16_t>(BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        static_cast<uint16_t>(BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        BLE_GATT_CHR_F_READ,
        static_cast<uint16_t>(BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        static_cast<uint16_t>(BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        static_cast<uint16_t>(BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        static_cast<uint16_t>(BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        static_cast<uint16_t>(BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
        static_cast<uint16_t>(BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC),
    };
    for (size_t i = 0; i < std::size(ids); ++i) {
        g_custom_uuids[i] = custom_uuid(static_cast<uint8_t>(0x10 + i));
        g_custom_chars[i].uuid = &g_custom_uuids[i].u;
        g_custom_chars[i].access_cb = gatt_access;
        g_custom_chars[i].arg = reinterpret_cast<void *>(static_cast<uintptr_t>(ids[i]));
        g_custom_chars[i].flags = flags[i];
        if (ids[i] == Status) g_custom_chars[i].val_handle = &status_value_handle_;
    }

    g_services[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    g_standard_service_uuids[0].u.type = BLE_UUID_TYPE_16;
    g_standard_service_uuids[0].value = 0x180F;
    g_services[0].uuid = &g_standard_service_uuids[0].u;
    g_services[0].characteristics = g_battery_chars;
    g_services[1].type = BLE_GATT_SVC_TYPE_PRIMARY;
    g_standard_service_uuids[1].u.type = BLE_UUID_TYPE_16;
    g_standard_service_uuids[1].value = 0x181A;
    g_services[1].uuid = &g_standard_service_uuids[1].u;
    g_services[1].characteristics = g_environment_chars;
    g_services[2].type = BLE_GATT_SVC_TYPE_PRIMARY;
    g_standard_service_uuids[2].u.type = BLE_UUID_TYPE_16;
    g_standard_service_uuids[2].value = 0x180A;
    g_services[2].uuid = &g_standard_service_uuids[2].u;
    g_services[2].characteristics = g_device_chars;
    g_services[3].type = BLE_GATT_SVC_TYPE_PRIMARY;
    g_services[3].uuid = &kCustomService.u;
    g_services[3].characteristics = g_custom_chars;
}

int BleManager::init(const char *device_name) {
    active_ = this;
    if (nimble_port_init() != ESP_OK) return -1;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(device_name && *device_name ? device_name : "Neko");
    build_gatt_table();
    if (ble_gatts_count_cfg(g_services) != 0 || ble_gatts_add_svcs(g_services) != 0) return -1;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_store_config_init();
    nimble_port_freertos_init(host_task);
    return 0;
}

void BleManager::host_task(void *) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void BleManager::on_sync() {
    if (!active_) return;
    ble_hs_id_infer_auto(0, &active_->own_address_type_);
    active_->start_advertising();
}

void BleManager::start_advertising() {
    ble_hs_adv_fields fields{};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    const char *name = ble_svc_gap_device_name();
    fields.name = reinterpret_cast<const uint8_t *>(name);
    fields.name_len = std::strlen(name);
    fields.name_is_complete = 1;
    fields.uuids128 = const_cast<ble_uuid128_t *>(&kCustomService);
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;
    ble_gap_adv_set_fields(&fields);
    ble_gap_adv_params params{};
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    params.itvl_min = 400;  // 250 ms while awake
    params.itvl_max = 800;
    ble_gap_adv_start(own_address_type_, nullptr, BLE_HS_FOREVER, &params, gap_event, this);
}

int BleManager::gap_event(ble_gap_event *event, void *arg) {
    auto *self = static_cast<BleManager *>(arg);
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            self->connection_handle_ = event->connect.conn_handle;
            ble_gap_security_initiate(self->connection_handle_);
            EventBus::instance().publish(make_event(EventType::BleConnected));
        } else self->start_advertising();
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        self->connection_handle_ = BLE_HS_CONN_HANDLE_NONE;
        EventBus::instance().publish(make_event(EventType::BleDisconnected));
        self->start_advertising();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        self->start_advertising();
        break;
    case BLE_GAP_EVENT_REPEAT_PAIRING: {
        ble_gap_conn_desc desc{};
        if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0) {
            ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }
    default: break;
    }
    return 0;
}

bool BleManager::encrypted(uint16_t conn_handle) const {
    ble_gap_conn_desc desc{};
    return ble_gap_conn_find(conn_handle, &desc) == 0 && desc.sec_state.encrypted;
}

void BleManager::open_provisioning_window(uint32_t seconds) {
    provisioning_until_us_ = esp_timer_get_time() +
        static_cast<int64_t>(std::clamp<uint32_t>(seconds, 30, 300)) * 1'000'000;
}

int BleManager::gatt_access(uint16_t conn_handle, uint16_t, ble_gatt_access_ctxt *ctxt, void *arg) {
    if (!active_) return BLE_ATT_ERR_UNLIKELY;
    return active_->access(conn_handle, ctxt, static_cast<CharacteristicId>(reinterpret_cast<uintptr_t>(arg)));
}

int BleManager::access(uint16_t conn_handle, ble_gatt_access_ctxt *ctxt, CharacteristicId id) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) return read_characteristic(ctxt, id);
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) return write_characteristic(conn_handle, ctxt, id);
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}

int BleManager::read_characteristic(ble_gatt_access_ctxt *ctxt, CharacteristicId id) {
    switch (id) {
    case Battery: { uint8_t p = battery_.status().percent; return append(ctxt, &p, 1); }
    case Temperature: { int16_t v = static_cast<int16_t>(sensor_.reading().temperature_c * 100); return append(ctxt, &v, 2); }
    case Humidity: { uint16_t v = static_cast<uint16_t>(std::max(0.0F, sensor_.reading().humidity_rh) * 100); return append(ctxt, &v, 2); }
    case Model: return append(ctxt, "ESP32-S3-RLCD-4.2", 19);
    case Firmware: return append(ctxt, "0.1.0", 5);
    case Protocol: { const uint8_t version[] = {1, 0}; return append(ctxt, version, sizeof(version)); }
    case Status: {
        struct __attribute__((packed)) Status { uint8_t version, flags, battery, reserved; int16_t temp, humidity, rssi; uint32_t epoch; } value{};
        const auto wifi = wifi_.status(); const auto env = sensor_.reading();
        value.version = 1; value.flags = (wifi.connected ? 1 : 0) | (connected() ? 2 : 0) | (env.valid ? 4 : 0);
        value.battery = battery_.status().percent; value.temp = static_cast<int16_t>(env.temperature_c * 100);
        value.humidity = static_cast<int16_t>(env.humidity_rh * 100); value.rssi = wifi.rssi;
        value.epoch = static_cast<uint32_t>(time_.now()); return append(ctxt, &value, sizeof(value));
    }
    case Time: { uint64_t value = static_cast<uint64_t>(time_.now()); return append(ctxt, &value, sizeof(value)); }
    case Timezone: { auto s = settings_.get(); return append(ctxt, s.timezone, std::strlen(s.timezone)); }
    case CatName: { auto s = settings_.get(); return append(ctxt, s.cat_name, std::strlen(s.cat_name)); }
    case Volume: { uint8_t value = audio_.status().volume; return append(ctxt, &value, 1); }
    case Diagnostics: {
        char json[160]; const auto bat = battery_.status(); const auto env = sensor_.reading();
        const int len = std::snprintf(json, sizeof(json), "{\"bat_v\":%.3f,\"bat_pct\":%u,\"temp\":%.2f,\"rh\":%.2f,\"wifi\":%d}",
            bat.voltage_v, bat.percent, env.temperature_c, env.humidity_rh, wifi_.status().connected);
        return append(ctxt, json, static_cast<size_t>(len));
    }
    default: return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
}

int BleManager::write_characteristic(uint16_t conn_handle, ble_gatt_access_ctxt *ctxt,
                                     CharacteristicId id) {
    if (!encrypted(conn_handle)) return BLE_ATT_ERR_INSUFFICIENT_ENC;
    uint8_t data[192]{}; uint16_t size = 0;
    int error = flatten(ctxt, data, sizeof(data), size); if (error) return error;
    switch (id) {
    case Time:
        if (size != 8) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        { uint64_t epoch = 0; std::memcpy(&epoch, data, 8); return time_.set_time(static_cast<time_t>(epoch), TimeSource::Network) == ESP_OK ? 0 : BLE_ATT_ERR_VALUE_NOT_ALLOWED; }
    case Timezone:
        if (!size || size >= 64) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        { auto value = settings_.get(); std::memcpy(value.timezone, data, size); value.timezone[size] = 0;
          if (settings_.update(value) != ESP_OK) return BLE_ATT_ERR_UNLIKELY;
          setenv("TZ", value.timezone, 1); tzset(); return 0; }
    case AlarmConfig:
        if (size < 4 || data[0] > 23 || data[1] > 59 || !data[2]) return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        { Alarm alarm{}; alarm.hour = data[0]; alarm.minute = data[1]; alarm.weekdays = data[2]; alarm.enabled = data[3] != 0;
          const size_t length = std::min<size_t>(size - 4, sizeof(alarm.label) - 1); std::memcpy(alarm.label, data + 4, length);
          return alarms_.set_alarm(alarm) == ESP_OK ? 0 : BLE_ATT_ERR_UNLIKELY; }
    case TimerConfig:
        if (size < 4) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        { uint32_t seconds = 0; std::memcpy(&seconds, data, 4); char label[32]{};
          const size_t length = std::min<size_t>(size - 4, sizeof(label) - 1); std::memcpy(label, data + 4, length);
          return timers_.start(seconds, label) == ESP_OK ? 0 : BLE_ATT_ERR_VALUE_NOT_ALLOWED; }
    case NotificationText:
        if (!size || size >= 96) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        data[size] = 0; return notifications_.push(reinterpret_cast<char *>(data)) == ESP_OK ? 0 : BLE_ATT_ERR_UNLIKELY;
    case CatName:
        if (!size || size >= 24) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        { auto value = settings_.get(); std::memcpy(value.cat_name, data, size); value.cat_name[size] = 0;
          return settings_.update(value) == ESP_OK ? 0 : BLE_ATT_ERR_UNLIKELY; }
    case WifiCredentials:
        if (esp_timer_get_time() > provisioning_until_us_) return BLE_ATT_ERR_INSUFFICIENT_AUTHOR;
        if (size < 3 || data[0] == 0 || data[0] > 32 || data[1] > 64 || size != 2 + data[0] + data[1]) return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        { char ssid[33]{}; char password[65]{}; std::memcpy(ssid, data + 2, data[0]); std::memcpy(password, data + 2 + data[0], data[1]);
          return wifi_.update_credentials(ssid, password) == ESP_OK ? 0 : BLE_ATT_ERR_UNLIKELY; }
    case Volume:
        if (size != 1 || data[0] > 100) return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        { auto value = settings_.get(); value.volume = data[0]; settings_.update(value); return audio_.set_volume(data[0]) == ESP_OK ? 0 : BLE_ATT_ERR_UNLIKELY; }
    case Control:
        if (size < 1) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        if (data[0] == 1) return audio_.play_tone(1200, 600) == ESP_OK ? 0 : BLE_ATT_ERR_UNLIKELY;
        if (data[0] == 2) { esp_restart(); return 0; }
        if (data[0] == 3) { EventBus::instance().publish(make_event(EventType::RequestRender, "diagnostics")); return 0; }
        if (data[0] == 4) { EventBus::instance().publish(make_event(EventType::OtaStarted, "BLE OTA request")); return 0; }
        return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
    default: return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    }
}

void BleManager::notify_status() {
    if (!connected()) return;
    if (status_value_handle_) ble_gatts_chr_updated(status_value_handle_);
    if (battery_value_handle_) ble_gatts_chr_updated(battery_value_handle_);
}

}  // namespace neko
