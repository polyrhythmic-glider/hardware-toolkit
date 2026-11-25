#pragma once

#include "NekoHardware.h"
#include "NekoScheduler.h"
#include "NekoTypes.h"
#include "NekoPhrases.h"
#include "ST7305_U8g2.h"

class NekoUi {
public:
  bool begin();
  void render(Page page, CatMood mood, const char *message, const Settings &settings,
              const NekoHardware &hardware, const NekoScheduler &scheduler,
              bool wifiConnected, int8_t rssi, bool bleConnected, bool timeValid);
  void powerSave(bool enabled);
  void dumpScreen();
  void connectionDetails(const char *ap, const char *password, bool setupOpen);
  void setDiagnosticSelection(unsigned selection) { diagnosticSelection_ = selection; }
  uint32_t lastTransferredTiles() const { return lastTransferredTiles_; }

private:
  void header(const char *title, const NekoHardware &hardware, bool wifi, bool ble);
  void footer(const char *hint);
  void drawPet(int x, int y, int size, CatMood mood);
  void speech(int mouthX, int mouthY, const char *message, const char *author);
  void drawHome(CatMood mood, const char *message, const Settings &settings,
                const NekoHardware &hardware, bool wifi, bool ble, bool timeValid,
                const NekoPhrases::Entry *phrase);
  void drawClock(bool timeValid, CatMood mood);
  void centered(int y, const char *text);
  void wrapped(int x, int y, int width, const char *text, int lines = 2);
  void card(int x, int y, int width, const char *title, const char *value);
  void row(int y, const char *label, const char *value);
  void flush();
  static const char *pageName(Page page);
  static const char *moodName(CatMood mood);

  ST7305_U8g2 lcd_{Board::LCD_SCLK, Board::LCD_MOSI, Board::LCD_DC, Board::LCD_CS, Board::LCD_RESET};
  U8G2 *u8g2_ = nullptr;
  std::array<uint8_t, 38 * 50 * 8> previous_{};
  std::array<uint8_t, 224 * 224 / 8> petBuffer_{};
  unsigned diagnosticSelection_ = 0;
  bool firstFrame_ = true;
  uint32_t lastTransferredTiles_ = 0;
};
