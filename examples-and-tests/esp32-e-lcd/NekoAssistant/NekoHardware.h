#pragma once

#include <Wire.h>
#include "NekoTypes.h"

class NekoHardware {
public:
  void begin(float sensorOffsetC);
  bool readEnvironment();
  void readBattery();
  bool readRtc(tm &value);
  bool writeRtc(time_t epoch);
  bool mountSd();
  bool appendLog(const char *line);
  bool i2cPresent(uint8_t address);

  const EnvironmentReading &environment() const { return environment_; }
  const BatteryReading &battery() const { return battery_; }
  bool rtcOk() const { return rtcOk_; }
  bool sdOk() const { return sdOk_; }
  bool audioCodecPresent() const { return audioCodecPresent_; }
  uint32_t i2cErrors() const { return i2cErrors_; }

private:
  bool command(uint8_t address, uint16_t value);
  bool readRegisters(uint8_t address, uint8_t reg, uint8_t *data, size_t size);
  static uint8_t crc8(const uint8_t *data, size_t size);
  static uint8_t toBcd(uint8_t value);
  static uint8_t fromBcd(uint8_t value);
  static uint8_t batteryPercent(float voltage);

  EnvironmentReading environment_{};
  BatteryReading battery_{};
  float sensorOffsetC_ = 1.5F;
  bool rtcOk_ = false;
  bool sdOk_ = false;
  bool audioCodecPresent_ = false;
  uint32_t i2cErrors_ = 0;
  std::array<float, 8> voltageHistory_{};
  uint8_t historyCount_ = 0;
  uint8_t historyIndex_ = 0;
};
