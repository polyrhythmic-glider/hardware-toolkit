#pragma once
#include <WebServer.h>
#include <Preferences.h>
#include "NekoConnectivity.h"

class NekoWeb {
public:
  NekoWeb(NekoSettings &settings, NekoHardware &hardware, NekoScheduler &scheduler, NekoConnectivity &connectivity)
    : settings_(settings), hardware_(hardware), scheduler_(scheduler), connectivity_(connectivity) {}
  void begin();
  void tick();
  void openSetupWindow(uint32_t seconds = 120);
  bool setupOpen() const { return setupUntil_ && static_cast<int32_t>(millis() - setupUntil_) < 0; }
  const char *apName() const { return apName_; }
  const char *password() const { return password_; }
  int takePageRequest() { const int page = pageRequest_; pageRequest_ = -1; return page; }
  bool takeChanged() { const bool changed = changed_; changed_ = false; return changed; }
  bool takePet() { const bool pet = pet_; pet_ = false; return pet; }
private:
  bool authenticate();
  void status();
  void action();
  static String json(const char *text);
  NekoSettings &settings_;
  NekoHardware &hardware_;
  NekoScheduler &scheduler_;
  NekoConnectivity &connectivity_;
  WebServer server_{80};
  char password_[17]{};
  char apName_[24]{};
  uint32_t setupUntil_ = 0;
  bool started_ = false, changed_ = false, pet_ = false, mdns_ = false, wasConnected_ = false;
  int pageRequest_ = -1;
};
