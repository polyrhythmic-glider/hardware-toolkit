#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// U8g2 R1 uses a physical 38x50 tile buffer. Diff physical memory directly,
// because updateDisplayArea deliberately ignores the logical rotation.
namespace NekoDisplayDiff {
template<class Send>
size_t update(const uint8_t *next, uint8_t *previous, unsigned columns, unsigned rows, bool first, Send send) {
  const size_t stride = columns * 8;
  size_t tiles = 0;
  for (unsigned row = 0; row < rows; ++row) {
    const auto *source = next + row * stride; auto *saved = previous + row * stride;
    unsigned left = 0, right = columns;
    if (!first) {
      while (left < columns && !memcmp(source + left * 8, saved + left * 8, 8)) ++left;
      if (left == columns) continue;
      while (right > left && !memcmp(source + (right - 1) * 8, saved + (right - 1) * 8, 8)) --right;
    }
    send(left, row, right - left);
    memcpy(saved, source, stride); tiles += right - left;
  }
  return tiles;
}
}
