#include "NekoConnectivity.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLESecurity.h>
#include <sys/time.h>

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
  beginWifi();
  beginBle();
}

void NekoConnectivity::beginWifi() {
  const Settings &settings = settings_.get();
  if (!settings.ssid[0]) {
    WiFi.mode(WIFI_OFF);
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.begin(settings.ssid, settings.password);
  configTzTime(settings.timezone, "pool.ntp.org", "time.cloudflare.com");
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

  BLEService *custom = server->createService(SERVICE);
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

bool NekoConnectivity::uuidEndsWith(const BLECharacteristic *characteristic, const char *prefix) {
  String value = characteristic->getUUID().toString();
  value.toLowerCase();
  return value.startsWith(prefix);
}

void NekoConnectivity::handleWrite(BLECharacteristic *characteristic) {
  const String value = characteristic->getValue();
  const uint8_t *data = reinterpret_cast<const uint8_t *>(value.c_str());
  const size_t size = value.length();
  Settings settings = settings_.get();
  if (uuidEndsWith(characteristic, "1200") && size == 8) {
    uint64_t epoch = 0; memcpy(&epoch, data, 8);
    timeval tv{static_cast<time_t>(epoch), 0}; settimeofday(&tv, nullptr); hardware_.writeRtc(tv.tv_sec);
  } else if (uuidEndsWith(characteristic, "1300") && size > 0 && size < sizeof(settings.timezone)) {
    memcpy(settings.timezone, data, size); settings.timezone[size] = 0; settings_.save(settings);
    setenv("TZ", settings.timezone, 1); tzset();
  } else if (uuidEndsWith(characteristic, "1400") && size >= 4) {
    AlarmSlot alarm{}; alarm.hour = data[0]; alarm.minute = data[1]; alarm.weekdays = data[2]; alarm.enabled = data[3];
    const size_t length = min(size - 4, sizeof(alarm.label) - 1); memcpy(alarm.label, data + 4, length); scheduler_.setAlarm(0, alarm);
  } else if (uuidEndsWith(characteristic, "1500") && size >= 4) {
    uint32_t seconds = 0; memcpy(&seconds, data, 4); char label[24]{};
    const size_t length = min(size - 4, sizeof(label) - 1); memcpy(label, data + 4, length); scheduler_.startTimer(seconds, label);
  } else if (uuidEndsWith(characteristic, "1600") && size > 0 && size < 96) {
    char text[96]{}; memcpy(text, data, size); scheduler_.pushNotification(text);
  } else if (uuidEndsWith(characteristic, "1700") && size > 0 && size < sizeof(settings.catName)) {
    memcpy(settings.catName, data, size); settings.catName[size] = 0; settings_.save(settings);
  } else if (uuidEndsWith(characteristic, "1800") && static_cast<int32_t>(millis() - provisioningUntilMs_) < 0 && size >= 3) {
    const uint8_t ssidLength = data[0], passwordLength = data[1];
    if (ssidLength && ssidLength <= 32 && passwordLength <= 64 && size == 2U + ssidLength + passwordLength) {
      memcpy(settings.ssid, data + 2, ssidLength); settings.ssid[ssidLength] = 0;
      memcpy(settings.password, data + 2 + ssidLength, passwordLength); settings.password[passwordLength] = 0;
      settings_.save(settings); WiFi.disconnect(true); delay(50); beginWifi();
    }
  } else if (uuidEndsWith(characteristic, "1900") && size == 1 && data[0] <= 100) {
    settings.volume = data[0]; settings_.save(settings);
  } else if (uuidEndsWith(characteristic, "1a00") && size && data[0] == 2) {
    ESP.restart();
  }
}

void NekoConnectivity::openProvisioningWindow(uint32_t seconds) {
  provisioningUntilMs_ = millis() + constrain(seconds, 30UL, 300UL) * 1000UL;
}

void NekoConnectivity::tick() {
  if (!started_) return;
  if (timeValid() && !rtcWrittenAfterSync_ && wifiConnected()) {
    hardware_.writeRtc(time(nullptr));
    rtcWrittenAfterSync_ = true;
  }
  if (millis() - lastStatusMs_ < 5000) return;
  lastStatusMs_ = millis();
  const auto &battery = hardware_.battery();
  const auto &environment = hardware_.environment();
  batteryCharacteristic_->setValue(&battery.percent, 1);
  const int16_t temperature = static_cast<int16_t>(environment.temperatureC * 100);
  const uint16_t humidity = static_cast<uint16_t>(max(0.0F, environment.humidityRh) * 100);
  temperatureCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&temperature), sizeof(temperature));
  humidityCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&humidity), sizeof(humidity));
  struct __attribute__((packed)) Status { uint8_t version, flags, battery, reserved; int16_t temp, humidity, rssi; uint32_t epoch; } status{};
  status.version = 1; status.flags = (wifiConnected() ? 1 : 0) | (bleConnected_ ? 2 : 0) | (environment.valid ? 4 : 0);
  status.battery = battery.percent; status.temp = temperature; status.humidity = humidity; status.rssi = rssi(); status.epoch = time(nullptr);
  statusCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&status), sizeof(status));
  const uint64_t epoch = time(nullptr); timeCharacteristic_->setValue(reinterpret_cast<const uint8_t *>(&epoch), sizeof(epoch));
  timezoneCharacteristic_->setValue(settings_.get().timezone); nameCharacteristic_->setValue(settings_.get().catName);
  volumeCharacteristic_->setValue(&settings_.get().volume, 1);
  char diagnostics[160]; snprintf(diagnostics, sizeof(diagnostics), "{\"bat_v\":%.3f,\"bat_pct\":%u,\"temp\":%.2f,\"rh\":%.2f,\"i2c_err\":%lu}",
    battery.voltage, battery.percent, environment.temperatureC, environment.humidityRh, (unsigned long)hardware_.i2cErrors());
  diagnosticsCharacteristic_->setValue(diagnostics);
  if (bleConnected_) { batteryCharacteristic_->notify(); temperatureCharacteristic_->notify(); humidityCharacteristic_->notify(); statusCharacteristic_->notify(); }
}
