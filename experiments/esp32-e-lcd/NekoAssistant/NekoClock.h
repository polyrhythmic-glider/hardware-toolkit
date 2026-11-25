#pragma once
namespace NekoClock {
// Continuous Orbitron outlines, packed from the licensed font (not LED segments).
constexpr int Width = 80, Height = 172, Top = 64;
constexpr int Left[4] = {12, 98, 222, 308};
constexpr int ColonLeft = 196;
template<class Draw>
void draw(int hour, int minute, bool valid, Draw drawGlyph) {
  const int digits[4] = {hour / 10, hour % 10, minute / 10, minute % 10};
  for (int i = 0; i < 4; ++i) drawGlyph(Left[i], Top, valid ? digits[i] : 10, Width, Height);
}
}
