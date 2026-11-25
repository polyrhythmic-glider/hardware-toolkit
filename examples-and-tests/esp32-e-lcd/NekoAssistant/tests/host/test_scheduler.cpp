#include <cassert>
#include <cstdlib>
#include <cstring>
#include "NekoScheduler.h"
uint32_t fakeMillis = 0;
int main() {
  setenv("TZ", "UTC0", 1); tzset();
  NekoScheduler s; assert(s.begin());
  assert(s.startTimer(0, "bad") == 0);
  assert(s.startTimer(604801, "bad") == 0);
  for (int i = 0; i < 4; ++i) assert(s.startTimer(1, "same second") != 0);
  assert(s.startTimer(5, "full") == 0);
  fakeMillis = 1001; s.tick(1800000000);
  char label[24]; int finished = 0;
  while (s.takeTimerEvent(label, sizeof(label))) ++finished;
  assert(finished == 4); // no lost simultaneous completions
  const auto id = s.startTimer(120, "pause and reboot"); assert(id);
  fakeMillis += 31000; s.tick(1800000031);
  assert(s.pauseTimer(id)); assert(s.timers()[0].remainingSeconds == 89);
  fakeMillis += 5000; s.tick(1800000036); assert(s.timers()[0].remainingSeconds == 89);
  assert(s.resumeTimer(id));
  fakeMillis += 61000; s.tick(1800000097); // NVS checkpoint preserves remaining time
  NekoScheduler reboot; assert(reboot.begin());
  assert(reboot.timers()[0].paused); assert(reboot.timers()[0].remainingSeconds == 28);
  assert(reboot.cancelTimer(id));
  assert(s.addReminder(time(nullptr) + 20, "Drink water"));
  NekoScheduler saved; assert(saved.begin()); assert(saved.reminders()[0].active);
  saved.tick(time(nullptr) + 21); assert(!saved.reminders()[0].active);
  assert(saved.notification().active); assert(!strcmp(saved.notification().text, "Drink water"));
  assert(!saved.addReminder(time(nullptr) - 1, "past"));
  assert(!saved.removeReminder(4));
  assert(!saved.pushNotification("Drink water")); // deduplicates for five minutes
  saved.dismissNotification(); assert(!saved.notification().active);
  AlarmSlot alarm{}; alarm.enabled = true;
  time_t now = 1800000000; tm utc{}; localtime_r(&now, &utc);
  alarm.hour = utc.tm_hour; alarm.minute = utc.tm_min; alarm.weekdays = 1 << utc.tm_wday;
  strcpy(alarm.label, "weekly"); assert(saved.setAlarm(0, alarm));
  saved.tick(now); assert(saved.takeAlarmEvent(label, sizeof(label))); assert(!strcmp(label, "weekly"));
  saved.tick(now + 1); assert(!saved.takeAlarmEvent(label, sizeof(label)));
  alarm.hour = 25; assert(!saved.setAlarm(0, alarm));
  NekoScheduler wrap; assert(wrap.begin());
  fakeMillis = 0xFFFFFF00u; const auto wrapId = wrap.startTimer(1, "wrap"); assert(wrapId);
  fakeMillis += 1001; wrap.tick(now); assert(wrap.takeTimerEvent(label, sizeof(label)));
}
