#include "NekoHardware.h"

#include <SD_MMC.h>
#include <time.h>

void NekoHardware::begin(float sensorOffsetC) {
  sensorOffsetC_ = sensorOffsetC;
  Wire.begin(Board::I2C_SDA, Board::I2C_SCL, 400000);
  analogReadResolution(12);
  analogSetPinAttenuation(Board::BATTERY_ADC, ADC_11db);
  rtcOk_ = i2cPresent(Board::RTC_ADDRESS);
  audioCodecPresent_ = i2cPresent(0x18) && i2cPresent(0x40);
  readEnvironment();
  readBattery();
}

bool NekoHardware::i2cPresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool NekoHardware::command(uint8_t address, uint16_t value) {
  Wire.beginTransmission(address);
  Wire.write(static_cast<uint8_t>(value >> 8));
  Wire.write(static_cast<uint8_t>(value));
  if (Wire.endTransmission() == 0) return true;
  ++i2cErrors_;
  return false;
}

bool NekoHardware::readRegisters(uint8_t address, uint8_t reg, uint8_t *data, size_t size) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(address, size) != size) {
    ++i2cErrors_;
    return false;
  }
  for (size_t i = 0; i < size; ++i) data[i] = Wire.read();
  return true;
}

uint8_t NekoHardware::crc8(const uint8_t *data, size_t size) {
  uint8_t crc = 0xFF;
  while (size--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
  }
  return crc;
}

bool NekoHardware::readEnvironment() {
  if (!command(Board::SHTC3_ADDRESS, 0x3517)) return environment_.valid = false;
  delay(1);
  if (!command(Board::SHTC3_ADDRESS, 0x7866)) return environment_.valid = false;
  delay(20);
  uint8_t data[6]{};
  const size_t count = Wire.requestFrom(Board::SHTC3_ADDRESS, sizeof(data));
  for (size_t i = 0; i < count && i < sizeof(data); ++i) data[i] = Wire.read();
  command(Board::SHTC3_ADDRESS, 0xB098);
  if (count != sizeof(data) || crc8(data, 2) != data[2] || crc8(data + 3, 2) != data[5]) {
    ++i2cErrors_;
    return environment_.valid = false;
  }
  const uint16_t rawT = (data[0] << 8) | data[1];
  const uint16_t rawH = (data[3] << 8) | data[4];
  environment_.temperatureC = -45.0F + 175.0F * rawT / 65536.0F - sensorOffsetC_;
  environment_.humidityRh = 100.0F * rawH / 65536.0F;
  environment_.valid = environment_.temperatureC > -40 && environment_.temperatureC < 100 &&
                       environment_.humidityRh >= 0 && environment_.humidityRh <= 100;
  environment_.sampledAt = millis();
  return environment_.valid;
}

uint8_t NekoHardware::batteryPercent(float voltage) {
  struct Point { float voltage; uint8_t percent; };
  static constexpr Point curve[] = {
    {3.20F, 0}, {3.45F, 5}, {3.55F, 10}, {3.65F, 20}, {3.72F, 35},
    {3.78F, 50}, {3.84F, 60}, {3.92F, 75}, {4.02F, 90}, {4.15F, 100}
  };
  if (voltage <= curve[0].voltage) return 0;
  for (size_t i = 1; i < std::size(curve); ++i) {
    if (voltage <= curve[i].voltage) {
      const float f = (voltage - curve[i - 1].voltage) / (curve[i].voltage - curve[i - 1].voltage);
      const float raw = curve[i - 1].percent + f * (curve[i].percent - curve[i - 1].percent);
      return constrain(static_cast<int>((raw + 2.5F) / 5) * 5, 0, 100);
    }
  }
  return 100;
}

void NekoHardware::readBattery() {
  uint32_t millivolts = 0;
  for (uint8_t i = 0; i < 16; ++i) millivolts += analogReadMilliVolts(Board::BATTERY_ADC);
  battery_.voltage = millivolts / 16.0F * 0.003F;  // divider 200k/100k
  battery_.percent = batteryPercent(battery_.voltage);
  battery_.low = battery_.percent <= 15;
  battery_.critical = battery_.percent <= 5 || battery_.voltage < 3.30F;
  voltageHistory_[historyIndex_++ % voltageHistory_.size()] = battery_.voltage;
  if (historyCount_ < voltageHistory_.size()) ++historyCount_;
  if (historyCount_ == voltageHistory_.size()) {
    const uint8_t oldest = historyIndex_ % voltageHistory_.size();
    battery_.chargingEstimate = battery_.voltage - voltageHistory_[oldest] > 0.025F;
  } else battery_.chargingEstimate = false;
}

uint8_t NekoHardware::toBcd(uint8_t value) { return (value / 10 << 4) | value % 10; }
uint8_t NekoHardware::fromBcd(uint8_t value) { return (value >> 4) * 10 + (value & 0x0F); }

bool NekoHardware::readRtc(tm &value) {
  uint8_t data[7]{};
  if (!readRegisters(Board::RTC_ADDRESS, 0x04, data, sizeof(data)) || (data[0] & 0x80)) {
    rtcOk_ = false;
    return false;
  }
  value = {};
  value.tm_sec = fromBcd(data[0] & 0x7F);
  value.tm_min = fromBcd(data[1] & 0x7F);
  value.tm_hour = fromBcd(data[2] & 0x3F);
  value.tm_mday = fromBcd(data[3] & 0x3F);
  value.tm_wday = fromBcd(data[4] & 0x07);
  value.tm_mon = fromBcd(data[5] & 0x1F) - 1;
  value.tm_year = fromBcd(data[6]) + 100;
  value.tm_isdst = -1;
  rtcOk_ = true;
  return true;
}

bool NekoHardware::writeRtc(time_t epoch) {
  tm local{};
  localtime_r(&epoch, &local);
  const uint8_t data[] = {
    0x04, toBcd(local.tm_sec), toBcd(local.tm_min), toBcd(local.tm_hour),
    toBcd(local.tm_mday), toBcd(local.tm_wday), toBcd(local.tm_mon + 1),
    toBcd((local.tm_year + 1900) % 100)
  };
  Wire.beginTransmission(Board::RTC_ADDRESS);
  Wire.write(data, sizeof(data));
  rtcOk_ = Wire.endTransmission() == 0;
  if (!rtcOk_) ++i2cErrors_;
  return rtcOk_;
}

bool NekoHardware::mountSd() {
  if (sdOk_) return true;
  if (!SD_MMC.setPins(Board::SD_CLK, Board::SD_CMD, Board::SD_D0)) return false;
  sdOk_ = SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT, 4);
  return sdOk_;
}

bool NekoHardware::appendLog(const char *line) {
  if (!sdOk_ || !line) return false;
  File file = SD_MMC.open("/neko.log", FILE_APPEND);
  if (!file) return false;
  const bool ok = file.println(line) > 0;
  file.close();
  return ok;
}
