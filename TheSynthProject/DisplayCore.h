#pragma once

// =======================
// FAST DRAW HELPERS
// =======================

int textPixelWidth(const char* text, uint8_t size) {
  return strlen(text) * 6 * size;
}

uint16_t getAccentColor(int itemIndex) {
  if (itemIndex < 0) return COL_NOTE;
  return ITEM_COLORS[itemIndex % ITEM_COLOR_COUNT];
}

void printRight(const char* text, int rightX, int y, uint8_t size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.setCursor(rightX - textPixelWidth(text, size), y);
  tft.print(text);
}
