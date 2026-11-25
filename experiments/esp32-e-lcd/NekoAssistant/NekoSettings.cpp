#include "NekoSettings.h"

namespace {
constexpr uint16_t kSchema = 1;
struct StoredSettings {
  uint16_t schema;
  uint16_t size;
  Settings value;
};
}

bool NekoSettings::begin() {
  if (!preferences_.begin("neko_cfg", false)) return false;
  StoredSettings stored{};
  const size_t read = preferences_.getBytes("settings", &stored, sizeof(stored));
  if (read == sizeof(stored) && stored.schema == kSchema && stored.size == sizeof(Settings)) {
    value_ = stored.value;
    value_.catName[sizeof(value_.catName) - 1] = 0;
    value_.timezone[sizeof(value_.timezone) - 1] = 0;
    value_.ssid[sizeof(value_.ssid) - 1] = 0;
    value_.password[sizeof(value_.password) - 1] = 0;
    return true;
  }
  value_ = Settings{};
  return save(value_);
}

bool NekoSettings::save(const Settings &value) {
  value_ = value;
  value_.catName[sizeof(value_.catName) - 1] = 0;
  value_.timezone[sizeof(value_.timezone) - 1] = 0;
  value_.ssid[sizeof(value_.ssid) - 1] = 0;
  value_.password[sizeof(value_.password) - 1] = 0;
  const StoredSettings stored{kSchema, sizeof(Settings), value_};
  return preferences_.putBytes("settings", &stored, sizeof(stored)) == sizeof(stored);
}

bool NekoSettings::factoryReset() {
  if (!preferences_.clear()) return false;
  value_ = Settings{};
  return save(value_);
}
