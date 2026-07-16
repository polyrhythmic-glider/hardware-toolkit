#pragma once

#include "NekoHardware.h"
#include "NekoScheduler.h"
#include "NekoTypes.h"
#include "ST7305_U8g2.h"

class NekoUi {
public:
  bool begin();
  void render(Page page, CatMood mood, const char *message, const Settings &settings,
              const NekoHardware &hardware, const NekoScheduler &scheduler,
              bool wifiConnected, int8_t rssi, bool bleConnected, bool timeValid);
  void powerSave(bool enabled);

private:
  void header(const char *title, const NekoHardware &hardware, bool wifi, bool ble);
  void footer(const char *hint);
  void drawCat(int cx, int cy, int scale, CatMood mood);
  void drawHome(CatMood mood, const char *message, const Settings &settings,
                const NekoHardware &hardware, bool wifi, bool ble, bool timeValid);
  void centered(int y, const char *text);
  static const char *pageName(Page page);
  static const char *moodName(CatMood mood);

  ST7305_U8g2 lcd_{Board::LCD_SCLK, Board::LCD_MOSI, Board::LCD_DC, Board::LCD_CS, Board::LCD_RESET};
  U8G2 *u8g2_ = nullptr;
};
