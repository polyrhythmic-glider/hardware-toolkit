#pragma once
#include "NekoConnectivity.h"
#include "NekoHardware.h"
#include "NekoScheduler.h"
#include "NekoSettings.h"
#include "NekoUi.h"
#include "NekoButton.h"
#include "NekoWeb.h"
#include "NekoNavigation.h"

class NekoApp {
public:
  NekoApp();
  void begin();
  void tick();
private:
  void pollButtons();
  void onDoublePress();
  void bootstrapTime();
  void updateMood();
  void render(const char *message = nullptr);
  void applyPowerProfile();
  void checkSdUpdate();
  void serialTick();
  void serialCommand(char *command);
  NekoSettings settings_;
  NekoHardware hardware_;
  NekoScheduler scheduler_;
  NekoConnectivity connectivity_;
  NekoWeb web_;
  NekoUi ui_;
  NekoButton key_, boot_;
  NekoChord chord_;
  bool diagnosticOpen_ = false;
  unsigned diagnosticSelection_ = 0;
  Page page_ = Page::Home;
  CatMood mood_ = CatMood::Idle;
  PowerProfile appliedProfile_ = PowerProfile::Balanced;
  char message_[96] = "Un piccolo compagno, sempre qui.";
  char serialLine_[256]{};
  uint16_t serialLength_ = 0;
  bool serialOverflow_ = false;
  uint32_t moodUntilMs_ = 0, lastEnvironmentMs_ = 0, lastBatteryMs_ = 0, lastRenderMs_ = 0;
  uint32_t lastRevision_ = 0, lastNotification_ = 0;
};
