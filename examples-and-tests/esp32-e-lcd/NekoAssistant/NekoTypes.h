#pragma once

#include <Arduino.h>
#include <array>

// ESP32-S3-WROOM-1-N16R8: flash Quad (QIO), PSRAM Octal (OPI). Con il profilo
// QSPI Arduino inizializza quad_psram e il boot stampa "chip is not connected".
#if defined(BOARD_HAS_PSRAM) && defined(CONFIG_SPIRAM_MODE_QUAD)
#error "Neko: seleziona Tools > PSRAM > OPI PSRAM (non QSPI PSRAM). In alternativa scegli Disabled."
#endif

namespace Board {
constexpr uint8_t LCD_SCLK = 11;
constexpr uint8_t LCD_MOSI = 12;
constexpr uint8_t LCD_DC = 5;
constexpr uint8_t LCD_CS = 40;
constexpr uint8_t LCD_RESET = 41;
constexpr uint8_t LCD_TE = 6;
constexpr uint8_t I2C_SDA = 13;
constexpr uint8_t I2C_SCL = 14;
constexpr uint8_t KEY = 18;
constexpr uint8_t BOOT_KEY = 0;
constexpr uint8_t BATTERY_ADC = 4;
constexpr uint8_t SD_CLK = 38;
constexpr uint8_t SD_CMD = 21;
constexpr uint8_t SD_D0 = 39;
constexpr uint8_t I2S_MCLK = 16;
constexpr uint8_t I2S_BCLK = 9;
constexpr uint8_t I2S_WS = 45;
constexpr uint8_t I2S_DIN = 10;
constexpr uint8_t I2S_DOUT = 8;
constexpr uint8_t PA_ENABLE = 46;
constexpr uint8_t RTC_ADDRESS = 0x51;
constexpr uint8_t SHTC3_ADDRESS = 0x70;
}  // namespace Board

enum class Page : uint8_t {
  Home, Clock, Timers, Alarms, Reminders, Weather, Environment, Notifications,
  Audio, Storage, Connectivity, Battery, Settings, Diagnostics, About, Count
};

enum class CatMood : uint8_t {
  Idle, Happy, Sleepy, Alert, Listening, Thinking, Speaking, Hot, Humid,
  LowBattery, Offline, Timer, Alarm, Error, Updating
};

enum class PowerProfile : uint8_t { Performance, Balanced, Saver, Night, Critical };

struct Settings {
  char catName[24] = "Neko";
  char timezone[64] = "CET-1CEST,M3.5.0,M10.5.0/3";
  char ssid[33] = "";
  char password[65] = "";
  uint8_t volume = 45;
  PowerProfile powerProfile = PowerProfile::Balanced;
  float sensorOffsetC = 1.5F;
};

struct EnvironmentReading {
  float temperatureC = NAN;
  float humidityRh = NAN;
  bool valid = false;
  uint32_t sampledAt = 0;
};

struct BatteryReading {
  float voltage = 0;
  uint8_t percent = 0;
  bool low = false;
  bool critical = false;
  bool chargingEstimate = false;
};

struct TimerSlot {
  bool active = false;
  bool paused = false;
  uint32_t id = 0;
  uint32_t remainingSeconds = 0;
  uint32_t deadlineMs = 0;
  char label[24] = "Timer";
};

struct AlarmSlot {
  bool enabled = false;
  uint8_t hour = 7;
  uint8_t minute = 30;
  uint8_t weekdays = 0x7F;
  char label[24] = "Sveglia";
  int32_t lastDay = -1;
};

struct NotificationSlot {
  bool active = false;
  uint8_t priority = 1;
  uint32_t expiresMs = 0;
  char text[96] = "";
};
