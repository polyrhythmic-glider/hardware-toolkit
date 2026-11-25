#include <array>
#include <cassert>
#include <cstring>
#include "NekoPetAssets.h"
#include "NekoPet.h"
#include "NekoDisplayDiff.h"
int main() {
  std::array<uint8_t, 38 * 50 * 8> current{}, previous{};
  unsigned calls = 0;
  auto full = [&](unsigned x, unsigned y, unsigned w) { assert(x == 0 && y < 50 && w == 38); ++calls; };
  assert(NekoDisplayDiff::update(current.data(), previous.data(), 38, 50, true, full) == 1900);
  assert(calls == 50);
  calls = 0;
  assert(NekoDisplayDiff::update(current.data(), previous.data(), 38, 50, false, full) == 0);
  assert(calls == 0); // an identical screen must cause no SPI display writes
  current[25 * 304 + 11 * 8 + 2] = 42;
  auto one = [&](unsigned x, unsigned y, unsigned w) { assert(x == 11 && y == 25 && w == 1); ++calls; };
  assert(NekoDisplayDiff::update(current.data(), previous.data(), 38, 50, false, one) == 1);
  assert(calls == 1 && current == previous);
  // Copy two actual dithered sprites into the physical rotated framebuffer.
  current.fill(0); previous.fill(0);
  std::array<uint8_t,2048> moved{};
  NekoPet::raster(NekoPetAssets::Frame128_0,128,NekoPet::sample(NekoPet::Pose::Idle,1200),moved.data());
  for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
    const int px = 299 - (118 + y), py = 12 + x;
    const size_t at = (py / 8) * 304 + px;
    const uint8_t bit = 1U << (py % 8);
    if (NekoPetAssets::Frames128[0][y * 16 + x / 8] & (1U << (x % 8))) previous[at] |= bit;
    if (moved.data()[y * 16 + x / 8] & (1U << (x % 8))) current[at] |= bit;
  }
  const auto changed = NekoDisplayDiff::update(current.data(), previous.data(), 38, 50, false,
    [&](unsigned x, unsigned y, unsigned w) { assert(x >= 6 && x + w <= 23 && y >= 1 && y <= 17); });
  assert(changed > 0 && changed < 300);
  assert(current == previous);
}
