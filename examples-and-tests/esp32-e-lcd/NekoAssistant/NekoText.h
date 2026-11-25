#pragma once
#include <cstddef>
#include <cstring>

// Pixel-width wrapping shared by the UI and host checks. UTF-8 characters and
// words stay intact; callers can check overflow instead of silently losing text.
namespace NekoText {
struct Line { char text[192]{}; };
struct Result { unsigned count = 0; bool overflow = false; };
template<class Measure>
Result wrap(const char *text, int width, Line *lines, unsigned capacity, Measure measure) {
  Result result;
  const char *cursor = text ? text : "";
  while (*cursor == ' ') ++cursor;
  while (*cursor && result.count < capacity) {
    auto &buffer = lines[result.count].text;
    size_t length = 0, space = 0;
    while (cursor[length] && length < sizeof(buffer) - 5) {
      const unsigned char lead = cursor[length];
      const size_t count = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;
      if (std::strlen(cursor + length) < count) break;
      std::memcpy(buffer + length, cursor + length, count); buffer[length + count] = 0;
      if (measure(buffer) > width) { buffer[length] = 0; break; }
      if (cursor[length] == ' ') space = length;
      length += count;
    }
    if (cursor[length] && space) { length = space; buffer[length] = 0; }
    if (!length) { result.overflow = true; return result; }
    ++result.count; cursor += length;
    while (*cursor == ' ') ++cursor;
  }
  result.overflow = *cursor != 0;
  return result;
}
}
