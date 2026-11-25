#include <cassert>
#include "NekoButton.h"
using Event = NekoButton::Event;
int main() {
  NekoButton b;
  assert(b.update(true, 0) == Event::None);
  assert(b.update(false, 10) == Event::None); // electrical bounce
  assert(b.update(false, 100) == Event::None);
  assert(b.update(true, 200) == Event::None);
  assert(b.update(true, 230) == Event::None);
  assert(b.update(false, 300) == Event::None);
  assert(b.update(false, 330) == Event::None);
  assert(b.update(false, 631) == Event::Click);
  assert(b.update(false, 700) == Event::None);
  NekoButton twice;
  twice.update(true, 100); twice.update(true, 130); twice.update(false, 200); twice.update(false, 230);
  twice.update(true, 300); twice.update(true, 330); twice.update(false, 400);
  assert(twice.update(false, 430) == Event::Double);
  assert(twice.update(false, 900) == Event::None);
  NekoButton held;
  held.update(true, 0); held.update(true, 30);
  assert(held.update(true, 1000) == Event::None); // release decides long vs very long
  assert(held.update(true, 5030) == Event::VeryLong);
  assert(held.update(true, 6000) == Event::None);
  held.update(false, 6100); assert(held.update(false, 6130) == Event::None);
  NekoButton longer;
  longer.update(true, 0); longer.update(true, 30); longer.update(false, 1100);
  assert(longer.update(false, 1130) == Event::Long);
  NekoButton rollover;
  rollover.update(true, 0xFFFFFF00u); rollover.update(true, 0xFFFFFF20u);
  rollover.update(false, 0xFFFFFF50u); rollover.update(false, 0xFFFFFF70u);
  assert(rollover.update(false, 0x00000100u) == Event::Click);
}
