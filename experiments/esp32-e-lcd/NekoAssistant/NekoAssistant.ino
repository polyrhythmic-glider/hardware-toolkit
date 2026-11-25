#include "NekoApp.h"

NekoApp neko;

void setup() {
  neko.begin();
}

void loop() {
  neko.tick();
}
