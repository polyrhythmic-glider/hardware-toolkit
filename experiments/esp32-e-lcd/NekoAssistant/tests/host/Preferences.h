#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
class Preferences {
  std::string prefix;
  inline static std::map<std::string, std::vector<unsigned char>> data;
public:
  bool begin(const char *name, bool) { prefix = std::string(name) + "/"; return true; }
  size_t getBytesLength(const char *key) { return data[prefix + key].size(); }
  size_t getBytes(const char *key, void *dest, size_t size) {
    auto &value = data[prefix + key]; if (value.size() != size) return 0;
    memcpy(dest, value.data(), size); return size;
  }
  size_t putBytes(const char *key, const void *source, size_t size) {
    const auto *first = static_cast<const unsigned char *>(source);
    data[prefix + key] = std::vector<unsigned char>(first, first + size); return size;
  }
};
