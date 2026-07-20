#pragma once

#include <Preferences.h>
#include "NekoTypes.h"

class NekoScheduler {
public:
  bool begin();
  uint32_t startTimer(uint32_t seconds, const char *label);
  bool pauseTimer(uint32_t id);
  bool resumeTimer(uint32_t id);
  bool cancelTimer(uint32_t id);
  bool setAlarm(uint8_t index, const AlarmSlot &alarm);
  bool pushNotification(const char *text, uint8_t priority = 1, uint32_t ttlSeconds = 30);
  void tick(time_t epoch);

  const std::array<TimerSlot, 4> &timers() const { return timers_; }
  const std::array<AlarmSlot, 8> &alarms() const { return alarms_; }
  const NotificationSlot &notification() const { return notification_; }
  bool takeTimerEvent(char *label, size_t size);
  bool takeAlarmEvent(char *label, size_t size);
  time_t nextAlarmEpoch(time_t now) const;

private:
  void save();
  static uint32_t hash(const char *text);

  Preferences preferences_;
  std::array<TimerSlot, 4> timers_{};
  std::array<AlarmSlot, 8> alarms_{};
  NotificationSlot notification_{};
  uint32_t nextTimerId_ = 1;
  bool timerEvent_ = false;
  bool alarmEvent_ = false;
  char timerEventLabel_[24]{};
  char alarmEventLabel_[24]{};
  uint32_t lastNotificationHash_ = 0;
  uint32_t lastNotificationMs_ = 0;
};
