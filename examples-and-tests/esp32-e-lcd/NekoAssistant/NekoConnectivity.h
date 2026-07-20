#pragma once

#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <WiFi.h>
#include "NekoHardware.h"
#include "NekoScheduler.h"
#include "NekoSettings.h"

class NekoConnectivity {
public:
  NekoConnectivity(NekoSettings &settings, NekoHardware &hardware, NekoScheduler &scheduler);
  void begin();
  void tick();
  void openProvisioningWindow(uint32_t seconds = 120);
  bool wifiConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool bleConnected() const { return bleConnected_; }
  int8_t rssi() const { return wifiConnected() ? static_cast<int8_t>(WiFi.RSSI()) : -127; }
  bool timeValid() const { return time(nullptr) >= 1700000000; }

private:
  class WriteCallbacks final : public BLECharacteristicCallbacks {
  public:
    explicit WriteCallbacks(NekoConnectivity &owner) : owner_(owner) {}
    void onWrite(BLECharacteristic *characteristic) override;
  private:
    NekoConnectivity &owner_;
  };
  class ServerCallbacks final : public BLEServerCallbacks {
  public:
    explicit ServerCallbacks(NekoConnectivity &owner) : owner_(owner) {}
    void onConnect(BLEServer *) override { owner_.bleConnected_ = true; }
    void onDisconnect(BLEServer *server) override;
  private:
    NekoConnectivity &owner_;
  };

  void beginWifi();
  void beginBle();
  void handleWrite(BLECharacteristic *characteristic);
  BLECharacteristic *make(BLEService *service, const char *uuid, uint32_t properties, bool encryptedWrite = false);
  static bool uuidEndsWith(const BLECharacteristic *characteristic, const char *prefix);

  NekoSettings &settings_;
  NekoHardware &hardware_;
  NekoScheduler &scheduler_;
  WriteCallbacks writeCallbacks_;
  ServerCallbacks serverCallbacks_;
  BLECharacteristic *batteryCharacteristic_ = nullptr;
  BLECharacteristic *temperatureCharacteristic_ = nullptr;
  BLECharacteristic *humidityCharacteristic_ = nullptr;
  BLECharacteristic *statusCharacteristic_ = nullptr;
  BLECharacteristic *timeCharacteristic_ = nullptr;
  BLECharacteristic *timezoneCharacteristic_ = nullptr;
  BLECharacteristic *nameCharacteristic_ = nullptr;
  BLECharacteristic *volumeCharacteristic_ = nullptr;
  BLECharacteristic *diagnosticsCharacteristic_ = nullptr;
  bool bleConnected_ = false;
  uint32_t provisioningUntilMs_ = 0;
  uint32_t lastStatusMs_ = 0;
  bool rtcWrittenAfterSync_ = false;
  bool started_ = false;
};
