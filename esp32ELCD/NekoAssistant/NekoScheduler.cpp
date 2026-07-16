#include "NekoScheduler.h"

#include <algorithm>

namespace {
struct SchedulerBlob {
  uint16_t schema = 1;
  uint16_t size = 0;
  std::array<TimerSlot, 4> timers{};
  std::array<AlarmSlot, 8> alarms{};
  uint32_t nextTimerId = 1;
};
}

bool NekoScheduler::begin() {
  if (!preferences_.begin("neko_sched", false)) return false;
  SchedulerBlob blob{};
  if (preferences_.getBytes("state", &blob, sizeof(blob)) == sizeof(blob) &&
      blob.schema == 1 && blob.size == sizeof(blob)) {
    timers_ = blob.timers;
    alarms_ = blob.alarms;
    nextTimerId_ = blob.nextTimerId;
    for (auto &timer : timers_) {
      if (timer.active) timer.paused = true;  // millis() non sopravvive al riavvio
      timer.deadlineMs = 0;
    }
  }
  return true;
}

void NekoScheduler::save() {
  SchedulerBlob blob{};
  blob.size = sizeof(blob);
  blob.timers = timers_;
  blob.alarms = alarms_;
  blob.nextTimerId = nextTimerId_;
  preferences_.putBytes("state", &blob, sizeof(blob));
}

uint32_t NekoScheduler::startTimer(uint32_t seconds, const char *label) {
  if (!seconds || seconds > 7U * 24U * 3600U) return 0;
  auto slot = std::find_if(timers_.begin(), timers_.end(), [](const auto &item) { return !item.active; });
  if (slot == timers_.end()) return 0;
  *slot = {};
  slot->active = true;
  slot->id = nextTimerId_++;
  slot->remainingSeconds = seconds;
  slot->deadlineMs = millis() + seconds * 1000UL;
  strlcpy(slot->label, label && *label ? label : "Timer", sizeof(slot->label));
  save();
  return slot->id;
}

bool NekoScheduler::pauseTimer(uint32_t id) {
  for (auto &timer : timers_) if (timer.active && timer.id == id && !timer.paused) {
    const int32_t remainingMs = static_cast<int32_t>(timer.deadlineMs - millis());
    timer.remainingSeconds = remainingMs > 0 ? (remainingMs + 999) / 1000 : 0;
    timer.paused = true;
    save();
    return true;
  }
  return false;
}

bool NekoScheduler::resumeTimer(uint32_t id) {
  for (auto &timer : timers_) if (timer.active && timer.id == id && timer.paused) {
    timer.deadlineMs = millis() + timer.remainingSeconds * 1000UL;
    timer.paused = false;
    save();
    return true;
  }
  return false;
}

bool NekoScheduler::cancelTimer(uint32_t id) {
  for (auto &timer : timers_) if (timer.active && timer.id == id) {
    timer = {};
    save();
    return true;
  }
  return false;
}

bool NekoScheduler::setAlarm(uint8_t index, const AlarmSlot &alarm) {
  if (index >= alarms_.size() || alarm.hour > 23 || alarm.minute > 59 || !alarm.weekdays) return false;
  alarms_[index] = alarm;
  alarms_[index].label[sizeof(alarms_[index].label) - 1] = 0;
  save();
  return true;
}

uint32_t NekoScheduler::hash(const char *text) {
  uint32_t value = 2166136261U;
  while (*text) value = (value ^ static_cast<uint8_t>(*text++)) * 16777619U;
  return value;
}

bool NekoScheduler::pushNotification(const char *text, uint8_t priority, uint32_t ttlSeconds) {
  if (!text || !*text || strlen(text) >= sizeof(notification_.text)) return false;
  const uint32_t value = hash(text);
  if (value == lastNotificationHash_ && millis() - lastNotificationMs_ < 300000UL) return false;
  notification_ = {};
  notification_.active = true;
  notification_.priority = min<uint8_t>(priority, 3);
  notification_.expiresMs = millis() + constrain(ttlSeconds, 5UL, 3600UL) * 1000UL;
  strlcpy(notification_.text, text, sizeof(notification_.text));
  lastNotificationHash_ = value;
  lastNotificationMs_ = millis();
  return true;
}

void NekoScheduler::tick(time_t epoch) {
  const uint32_t nowMs = millis();
  for (auto &timer : timers_) {
    if (!timer.active || timer.paused) continue;
    const int32_t remainingMs = static_cast<int32_t>(timer.deadlineMs - nowMs);
    timer.remainingSeconds = remainingMs > 0 ? (remainingMs + 999) / 1000 : 0;
    if (remainingMs <= 0) {
      strlcpy(timerEventLabel_, timer.label, sizeof(timerEventLabel_));
      timerEvent_ = true;
      timer = {};
      save();
    }
  }
  if (notification_.active && static_cast<int32_t>(nowMs - notification_.expiresMs) >= 0) notification_ = {};
  if (epoch < 1700000000) return;
  tm local{};
  localtime_r(&epoch, &local);
  const int32_t dayKey = local.tm_year * 366 + local.tm_yday;
  for (auto &alarm : alarms_) {
    if (!alarm.enabled || !(alarm.weekdays & (1U << local.tm_wday)) || alarm.lastDay == dayKey) continue;
    if (local.tm_hour == alarm.hour && local.tm_min == alarm.minute) {
      alarm.lastDay = dayKey;
      strlcpy(alarmEventLabel_, alarm.label, sizeof(alarmEventLabel_));
      alarmEvent_ = true;
      save();
    }
  }
}

bool NekoScheduler::takeTimerEvent(char *label, size_t size) {
  if (!timerEvent_) return false;
  strlcpy(label, timerEventLabel_, size);
  timerEvent_ = false;
  return true;
}

bool NekoScheduler::takeAlarmEvent(char *label, size_t size) {
  if (!alarmEvent_) return false;
  strlcpy(label, alarmEventLabel_, size);
  alarmEvent_ = false;
  return true;
}

time_t NekoScheduler::nextAlarmEpoch(time_t now) const {
  if (now < 1700000000) return 0;
  for (uint8_t days = 0; days < 8; ++days) {
    const time_t candidateDay = now + days * 86400;
    tm date{};
    localtime_r(&candidateDay, &date);
    time_t best = 0;
    for (const auto &alarm : alarms_) {
      if (!alarm.enabled || !(alarm.weekdays & (1U << date.tm_wday))) continue;
      tm candidate = date;
      candidate.tm_hour = alarm.hour;
      candidate.tm_min = alarm.minute;
      candidate.tm_sec = 0;
      const time_t epoch = mktime(&candidate);
      if (epoch > now && (!best || epoch < best)) best = epoch;
    }
    if (best) return best;
  }
  return 0;
}
