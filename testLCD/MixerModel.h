#pragma once

#include "MenuModel.h"

// Converte un'azione mixer wave nell'indice della voce corrispondente.
inline int getMixerWaveIndex(int action) {
  switch (action) {
    case ACTION_MIX_SINE:
      return 0;

    case ACTION_MIX_SQUARE:
      return 1;

    case ACTION_MIX_TRIANGLE:
      return 2;

    case ACTION_MIX_SAW:
      return 3;

    default:
      return -1;
  }
}

// Checksum compatto per invalidare la UI quando cambia un gruppo di fader.
inline int getLevelChecksum(const int levels[], int count) {
  int checksum = 0;

  for (int i = 0; i < count; i++) {
    checksum = checksum * 31 + levels[i];
  }

  return checksum;
}
