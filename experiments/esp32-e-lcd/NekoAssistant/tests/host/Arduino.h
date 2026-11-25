#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cmath>
#include <string>
extern uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }
using std::min;
using std::max;
template<class T, class L, class H> T constrain(T value, L low, H high) { return std::min<T>(high, std::max<T>(low, value)); }
inline size_t strlcpy(char *dest, const char *source, size_t size) {
  const size_t length = strlen(source);
  if (size) { const size_t count = std::min(length, size - 1); memcpy(dest, source, count); dest[count] = 0; }
  return length;
}
