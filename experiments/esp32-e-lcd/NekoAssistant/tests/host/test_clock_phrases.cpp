#include <cassert>
#include <cstring>
#include <set>
#include <string>
#include "NekoClock.h"
#include "NekoPhrases.h"
int main() {
  for (int h = 0; h < 24; ++h) for (int m = 0; m < 60; ++m) {
    unsigned count = 0;
    NekoClock::draw(h,m,true,[&](int x,int y,int digit,int width,int height) {
      assert(x >= 12 && x + width <= 388 && y == 64 && y + height <= 236);
      assert(width == 80 && height == 172 && digit >= 0 && digit <= 9); ++count;
    });
    assert(count == 4);
  }
  unsigned missing = 0;
  NekoClock::draw(0,0,false,[&](int,int,int digit,int,int) { assert(digit == 10); ++missing; });
  assert(missing == 4);
  std::set<std::string> unique,topics;
  static_assert(NekoPhrases::Count>=32);
  for (const auto &phrase : NekoPhrases::Entries) {
    assert(strlen(phrase.text) > 0 && strlen(phrase.text) < 120);
    assert(strlen(phrase.author) > 0 && !strncmp(phrase.source,"https://",8));
    assert(unique.insert(phrase.text).second);
    assert(phrase.topic&&strlen(phrase.topic)>0);topics.insert(phrase.topic);
  }
  assert(topics.size()>=8);
  for (uint32_t i = 0; i < NekoPhrases::Count; ++i) {
    assert(NekoPhrases::indexAt(i * NekoPhrases::IntervalMs) == i);
    assert(NekoPhrases::indexAt(i * NekoPhrases::IntervalMs + NekoPhrases::IntervalMs - 1) == i);
  }
  assert(NekoPhrases::indexAt(299999) == 0);
  assert(NekoPhrases::indexAt(300000) == 1);
  assert(NekoPhrases::indexAt(599999) == 1);
  assert(NekoPhrases::indexAt(600000) == 2);
  assert(NekoPhrases::indexAt(NekoPhrases::Count * NekoPhrases::IntervalMs) == 0);
  const auto *before = &NekoPhrases::current(12000); NekoPhrases::next();
  assert(&NekoPhrases::current(12000) != before);
  for (size_t i = 1; i < NekoPhrases::Count; ++i) NekoPhrases::next();
  assert(&NekoPhrases::current(12000) == before);
  assert(NekoPhrases::indexAt(0xffffffffU) < NekoPhrases::Count);
}
