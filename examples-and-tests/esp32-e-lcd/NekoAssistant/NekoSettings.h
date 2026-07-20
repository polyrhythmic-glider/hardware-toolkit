#pragma once

#include <Preferences.h>
#include "NekoTypes.h"

class NekoSettings {
public:
  bool begin();
  const Settings &get() const { return value_; }
  bool save(const Settings &value);
  bool factoryReset();

private:
  Preferences preferences_;
  Settings value_{};
};
