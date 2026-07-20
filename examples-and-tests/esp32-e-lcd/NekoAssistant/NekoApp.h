#pragma once

#include "NekoConnectivity.h"
#include "NekoHardware.h"
#include "NekoScheduler.h"
#include "NekoSettings.h"
#include "NekoUi.h"

class NekoApp {
public:
  NekoApp();
  void begin();
  void tick();

private:
  static void keyTaskEntry(void *argument);
  static void bootTaskEntry(void *argument);
  void keyTask();
  void bootTask();
  void dispatchButtonEvent(EventBits_t event, const char *source);
  void bootstrapTime();
  void onShortPress();
  void onDoublePress();
  void onLongPress();
  void onVeryLongPress();
  void updateMood();
  void render(const char *message = nullptr);
  void applyPowerProfile();
  void checkSdUpdate();

  NekoSettings settings_;
  NekoHardware hardware_;
  NekoScheduler scheduler_;
  NekoConnectivity connectivity_;
  NekoUi ui_;
  Page page_ = Page::Home;
  CatMood mood_ = CatMood::Idle;
  char message_[96] = "Buongiorno.";
  uint32_t moodUntilMs_ = 0;
  uint32_t lastEnvironmentMs_ = 0;
  uint32_t lastBatteryMs_ = 0;
  uint32_t lastRenderMs_ = 0;
  SemaphoreHandle_t displayMutex_ = nullptr;
  TaskHandle_t keyTaskHandle_ = nullptr;
  TaskHandle_t bootTaskHandle_ = nullptr;
};
