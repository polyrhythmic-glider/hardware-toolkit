#include "NekoConnectivity.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLESecurity.h>
#include <sys/time.h>
#include <esp_sntp.h>

NekoConnectivity *NekoConnectivity::active_ = nullptr;

namespace {
constexpr char SERVICE[] = "01004f4b-454e-a19e-6f4d-5280106e2b7a";
String uuid(uint8_t id) {
  char value[37];
  snprintf(value, sizeof(value), "%02x004f4b-454e-a19e-6f4d-5280106e2b7a", id);
  return String(value);
}
}

NekoConnectivity::NekoConnectivity(NekoSettings &settings, NekoHardware &hardware, NekoScheduler &scheduler)
    : settings_(settings), hardware_(hardware), scheduler_(scheduler),
      writeCallbacks_(*this), serverCallbacks_(*this) {}

void NekoConnectivity::begin() {
  active_ = this;
  commands_ = xQueueCreate(8, sizeof(Command));
  if (!commands_) { Serial.println("[BLE] command queue unavailable"); return; }
  Serial.println("[NEKO] starting Wi-Fi stack"); beginWifi();
  Serial.println("[NEKO] starting BLE"); beginBle();
  Serial.println("[NEKO] radio ready");
}

void NekoConnectivity::beginWifi() {
  const Settings &settings = settings_.get();
  if (!settings.ssid[0]) {
    // Initialize the station interface before the web socket and BLE host.
    // No association is attempted until credentials are supplied.
    WiFi.mode((WiFi.getMode() & WIFI_AP) ? WIFI_AP_STA : WIFI_STA);
    WiFi.setSleep(static_cast<unsigned>(settings.powerProfile) >= static_cast<unsigned>(PowerProfile::Saver));
    return;
  }
  WiFi.mode((WiFi.getMode() & WIFI_AP) ? WIFI_AP_STA : WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(static_cast<unsigned>(settings.powerProfile) >= static_cast<unsigned>(PowerProfile::Saver));
  WiFi.begin(settings.ssid, settings.password);
  sntp_set_time_sync_notification_cb([](timeval *) { if (active_) active_->rtcSyncPending_ = true; });
  configTzTime(settings.timezone, "pool.ntp.org", "time.cloudflare.com");
}

bool NekoConnectivity::configureWifi(const char *ssid, const char *password) {
  if (!ssid || !*ssid || strlen(ssid) > 32 || !password || strlen(password) > 64) return false;
  Settings value = settings_.get(); strlcpy(value.ssid, ssid, sizeof(value.ssid)); strlcpy(value.password, password, sizeof(value.password));
  if (!settings_.save(value)) return false;
  WiFi.disconnect(false); beginWifi(); changed_ = true; return true;
}

BLECharacteristic *NekoConnectivity::make(BLEService *service, const char *id, uint32_t properties,
                                           bool encryptedWrite) {
  BLECharacteristic *characteristic = service->createCharacteristic(id, properties);
  if (properties & BLECharacteristic::PROPERTY_NOTIFY) characteristic->addDescriptor(new BLE2902());
  if (properties & BLECharacteristic::PROPERTY_WRITE) {
    characteristic->setCallbacks(&writeCallbacks_);
    if (encryptedWrite) characteristic->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED);
  }
  return characteristic;
}

void NekoConnectivity::beginBle() {
  if (!BLEDevice::init(settings_.get().catName)) return;
  BLESecurity::setCapability(ESP_IO_CAP_NONE);
  BLESecurity::setAuthenticationMode(true, false, true);
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(&serverCallbacks_);
  server->advertiseOnDisconnect(true);

  BLEService *battery = server->createService(BLEUUID(static_cast<uint16_t>(0x180F)));
  batteryCharacteristic_ = make(battery, BLEUUID(static_cast<uint16_t>(0x2A19)).toString().c_str(),
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  battery->start();

  BLEService *environment = server->createService(BLEUUID(static_cast<uint16_t>(0x181A)));
  temperatureCharacteristic_ = make(environment, BLEUUID(static_cast<uint16_t>(0x2A6E)).toString().c_str(),
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  humidityCharacteristic_ = make(environment, BLEUUID(static_cast<uint16_t>(0x2A6F)).toString().c_str(),
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  environment->start();

  BLEService *custom = server->createService(BLEUUID(String(SERVICE)), 64);
  BLECharacteristic *protocol = make(custom, uuid(0x10).c_str(), BLECharacteristic::PROPERTY_READ);
  const uint8_t protocolVersion[] = {1, 0}; protocol->setValue(protocolVersion, sizeof(protocolVersion));
  statusCharacteristic_ = make(custom, uuid(0x11).c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  timeCharacteristic_ = make(custom, uuid(0x12).c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, true);
  timezoneCharacteristic_ = make(custom, uuid(0x13).c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, true);
  make(custom, uuid(0x14).c_str(), BLECharacteristic::PROPERTY_WRITE, true);
  make(custom, uuid(0x15).c_str(), BLECharacteristic::PROPERTY_WRITE, true);
  make(custom, uuid(0x16).c_str(), BLECharacteristic::PROPERTY_WRITE, true);
  nameCharacteristic_ = make(custom, uuid(0x17).c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, true);
  make(custom, uuid(0x18).c_str(), BLECharacteristic::PROPERTY_WRITE, true);
  volumeCharacteristic_ = make(custom, uuid(0x19).c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, true);
  make(custom, uuid(0x1A).c_str(), BLECharacteristic::PROPERTY_WRITE, true);
  diagnosticsCharacteristic_ = make(custom, uuid(0x1B).c_str(), BLECharacteristic::PROPERTY_READ);
  resultCharacteristic_ = make(custom, uuid(0x1C).c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  resultCharacteristic_->setValue("OK ready");
  custom->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  started_ = true;
}

void NekoConnectivity::ServerCallbacks::onDisconnect(BLEServer *server) {
  owner_.bleConnected_ = false;
  delay(20);
  server->startAdvertising();
}

void NekoConnectivity::WriteCallbacks::onWrite(BLECharacteristic *characteristic) {
  owner_.handleWrite(characteristic);
}

void NekoConnectivity::handleWrite(BLECharacteristic *characteristic) {
  const String value = characteristic->getValue();
  if (!commands_ || value.length() > 128) return;
  Command command{};
  const String id = characteristic->getUUID().toString();
  command.id = strtoul(id.substring(0, 2).c_str(), nullptr, 16);
  command.size = value.length();
  memcpy(command.data, value.c_str(), command.size);
  if (xQueueSend(commands_, &command, 0) != pdTRUE) Serial.println("[BLE] command queue full");
}

void NekoConnectivity::result(bool ok, const char *detail) {
  char message[96]; snprintf(message, sizeof(message), "%s %s", ok ? "OK" : "ERR", detail);
  resultCharacteristic_->setValue(message);
  if (bleConnected_) resultCharacteristic_->notify();
  changed_ = true;
}

void NekoConnectivity::processCommand(uint8_t id, const uint8_t *data, size_t size) {
  Settings settings = settings_.get();
  bool ok = true;
  if (id == 0x12 && size == 8) {
    uint64_t epoch = 0; memcpy(&epoch, data, 8);
    if (epoch < 1700000000ULL || epoch > 4102444800ULL) { result(false, "Data non valida"); return; }
    timeval tv{static_cast<time_t>(epoch), 0}; settimeofday(&tv, nullptr); hardware_.writeRtc(tv.tv_sec);
  } else if (id == 0x13 && size > 0 && size < sizeof(settings.timezone)) {
    memcpy(settings.timezone, data, size); settings.timezone[size] = 0; settings_.save(settings);
    setenv("TZ", settings.timezone, 1); tzset();
  } else if (id == 0x14 && size >= 4) {
    AlarmSlot alarm{}; alarm.hour = data[0]; alarm.minute = data[1]; alarm.weekdays = data[2]; alarm.enabled = data[3];
    const size_t length = min(size - 4, sizeof(alarm.label) - 1); memcpy(alarm.label, data + 4, length);
    const uint8_t index = (size >= 29) ? data[28] : 0;
    ok = scheduler_.setAlarm(index, alarm);
  } else if (id == 0x15 && size >= 4) {
    uint32_t seconds = 0; memcpy(&seconds, data, 4); char label[24]{};
    const size_t length = min(size - 4, sizeof(label) - 1); memcpy(label, data + 4, length); ok = scheduler_.startTimer(seconds, label) != 0;
  } else if (id == 0x16 && size > 0 && size < 96) {
    char text[96]{}; memcpy(text, data, size); ok = scheduler_.pushNotification(text, 1, 300);
  } else if (id == 0x17 && size > 0 && size < sizeof(settings.catName)) {
    memcpy(settings.catName, data, size); settings.catName[size] = 0; settings_.save(settings);
  } else if (id == 0x18 && static_cast<int32_t>(millis() - provisioningUntilMs_) < 0 && size >= 3) {
    const uint8_t ssidLength = data[0], passwordLength = data[1];
    if (ssidLength && ssidLength <= 32 && passwordLength <= 64 && size == 2U + ssidLength + passwordLength) {
      memcpy(settings.ssid, data + 2, ssidLength); settings.ssid[ssidLength] = 0;
      memcpy(settings.password, data + 2 + ssidLength, passwordLength); settings.password[passwordLength] = 0;
      ok = configureWifi(settings.ssid, settings.password);
    } else ok = false;
  } else if (id == 0x19 && size == 1 && data[0] <= 100) {
    settings.volume = data[0]; settings_.save(settings);
  } else if (id == 0x1A && size) {
    uint32_t timerId = 0;
    if (size == 5) memcpy(&timerId, data + 1, 4);
    switch (data[0]) {
      case 2: result(true, "Riavvio"); delay(100); ESP.restart(); break;
      case 3: pageRequest_ = static_cast<int>(Page::Diagnostics); break;
      case 4: pageRequest_ = static_cast<int>(Page::Cat); break;
      case 5: pageRequest_ = static_cast<int>(Page::Home); break;
      case 0x10: ok = size == 5 && scheduler_.pauseTimer(timerId); break;
      case 0x11: ok = size == 5 && scheduler_.resumeTimer(timerId); break;
      case 0x12: ok = size == 5 && scheduler_.cancelTimer(timerId); break;
      case 0x13:
        ok = size == 2 && data[1] <= static_cast<uint8_t>(PowerProfile::Night);
        if (ok) { settings.powerProfile = static_cast<PowerProfile>(data[1]); ok = settings_.save(settings); }
        break;
      case 0x14:
        ok = size > 9 && size < 73;
        if (ok) { uint64_t epoch = 0; memcpy(&epoch, data + 1, 8); char text[64]{};
          memcpy(text, data + 9, size - 9); ok = scheduler_.addReminder(static_cast<time_t>(epoch), text); }
        break;
      case 0x15: ok = size == 2 && scheduler_.removeReminder(data[1]); break;
      case 0x16: scheduler_.dismissNotification(); break;
      default: ok = false; break;
    }
  } else ok = false;
  result(ok, ok ? "Salvato" : "Comando non valido, memoria piena o finestra BLE chiusa");
}

void NekoConnectivity::openProvisioningWindow(uint32_t seconds) {
  provisioningUntilMs_ = millis() + constrain(seconds, 30UL, 300UL) * 1000UL;
}

void NekoConnectivity::tick() {
  if (!started_) return;
  Command command{};
  // Mutations, I2C and flash writes belong to this task, never the BLE host task.
  if (xQueueReceive(commands_, &command, 0) == pdTRUE) processCommand(command.id, command.data, command.size);
  if (rtcSyncPending_.exchange(false) && timeValid()) {
    hardware_.writeRtc(time(nullptr));
  }
  if (millis() - lastStatusMs_ < 5000) return;
  lastStatusMs_ = millis();
  const auto &battery = hardware_.battery();
  const auto &environment = hardware_.environment();
  batteryCharacteristic_->setValue(&battery.percent, 1);
  const int16_t temperature = environment.valid ? static_cast<int16_t>(environment.temperatureC * 100) : INT16_MIN;
  const uint16_t humidity = environment.valid ? static_cast<uint16_t>(environment.humidityRh * 100) : UINT16_MAX;
  temperatureCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&temperature), sizeof(temperature));
  humidityCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&humidity), sizeof(humidity));
  struct __attribute__((packed)) Status { uint8_t version, flags, battery, reserved; int16_t temp, humidity, rssi; uint32_t epoch; } status{};
  status.version = 1; status.flags = (wifiConnected() ? 1 : 0) | (bleConnected_ ? 2 : 0) | (environment.valid ? 4 : 0);
  status.battery = battery.percent; status.temp = temperature; status.humidity = humidity; status.rssi = rssi(); status.epoch = time(nullptr);
  statusCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&status), sizeof(status));
  const uint64_t epoch = time(nullptr); timeCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&epoch), sizeof(epoch));
  timezoneCharacteristic_->setValue(settings_.get().timezone); nameCharacteristic_->setValue(settings_.get().catName);
  volumeCharacteristic_->setValue(&settings_.get().volume, 1);
  char diagnostics[512];
  size_t used = snprintf(diagnostics, sizeof(diagnostics), "{\"version\":\"0.7.0\",\"bat_v\":%.3f,\"profile\":%u,\"i2c_err\":%lu,\"timers\":[",
    battery.voltage, static_cast<unsigned>(settings_.get().powerProfile), (unsigned long)hardware_.i2cErrors());
  bool first = true;
  for (const auto &timer : scheduler_.timers()) if (timer.active) {
    used += snprintf(diagnostics + used, sizeof(diagnostics) - used, "%s{\"id\":%lu,\"seconds\":%lu,\"paused\":%s}",
      first ? "" : ",", (unsigned long)timer.id, (unsigned long)timer.remainingSeconds, timer.paused ? "true" : "false");
    first = false;
  }
  snprintf(diagnostics + used, sizeof(diagnostics) - used, "]}");
  diagnosticsCharacteristic_->setValue(diagnostics);
  if (bleConnected_) { batteryCharacteristic_->notify(); temperatureCharacteristic_->notify(); humidityCharacteristic_->notify(); statusCharacteristic_->notify(); }
}
