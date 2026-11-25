#include "NekoUi.h"
#include "NekoPet.h"
#include "NekoPetAssets.h"
#include "NekoClockAssets.h"
#include "NekoBrandAssets.h"
#include "NekoNavigation.h"
#include "NekoDisplayDiff.h"
#include "NekoClock.h"
#include "NekoSpeech.h"
#include <SD_MMC.h>
#include <WiFi.h>

bool NekoUi::begin() {
  lcd_.begin(0, U8G2_R1);
  u8g2_ = lcd_.getU8g2();
  if (!u8g2_ || !u8g2_->getBufferPtr()) return false;
  u8g2_->setFontMode(1); u8g2_->setBitmapMode(1);
  return true;
}
void NekoUi::powerSave(bool enabled) { if (u8g2_) u8g2_->setPowerSave(enabled); }
void NekoUi::centered(int y, const char *text) {
  u8g2_->drawUTF8(max(8, (400 - u8g2_->getUTF8Width(text)) / 2), y, text);
}
const char *NekoUi::pageName(Page page) {
  static const char *names[] = {"HOME", "IL TUO NEKO", "OROLOGIO", "AMBIENTE", "TIMER", "SVEGLIE", "PROMEMORIA",
    "MESSAGGI", "CONNESSIONI", "ENERGIA", "PREFERENZE", "MICRO SD", "AUDIO", "DIAGNOSTICA", "SISTEMA"};
  static_assert(sizeof(names) / sizeof(names[0]) == static_cast<unsigned>(Page::Count));
  return names[static_cast<unsigned>(page)];
}
const char *NekoUi::moodName(CatMood mood) {
  static const char *names[] = {"Tutto tranquillo", "Purrr. Mi piace.", "Un piccolo riposo", "Occhi aperti", "Ti ascolto",
    "Un pensierino...", "Miao", "Un po' di caldo", "Aria umida", "Mi serve energia", "Riposo offline",
    "Il tempo vola", "Ehi, sveglia!", "Qualcosa non va", "Mi sto aggiornando"};
  return names[static_cast<unsigned>(mood)];
}
void NekoUi::header(const char *title, const NekoHardware &, bool, bool) {
  u8g2_->setFont(u8g2_font_9x18B_tf); u8g2_->drawUTF8(18, 30, title);
}
void NekoUi::footer(const char *hint) {
  u8g2_->drawHLine(12, 278, 376);
  u8g2_->setFont(u8g2_font_5x8_tf); u8g2_->drawUTF8(14, 292, hint);
}
void NekoUi::wrapped(int x, int y, int width, const char *text, int lines) {
  if (!text) return;
  const char *cursor = text;
  for (int line = 0; line < lines && *cursor; ++line) {
    char buffer[128]{}; size_t length = 0, lastSpace = 0;
    while (cursor[length] && length < sizeof(buffer) - 5) {
      const unsigned char lead = cursor[length];
      const size_t count = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
      if (length + count >= sizeof(buffer) - 5 || strlen(cursor + length) < count) break;
      memcpy(buffer + length, cursor + length, count); buffer[length + count] = 0;
      if (u8g2_->getUTF8Width(buffer) > width) { buffer[length] = 0; break; }
      if (cursor[length] == ' ') lastSpace = length;
      length += count;
    }
    if (!length) return;
    if (cursor[length] && lastSpace && line + 1 < lines) { length = lastSpace; buffer[length] = 0; }
    if (cursor[length] && line == lines - 1) {
      while (length && u8g2_->getUTF8Width(buffer) > width - 18) {
        --length; while (length && (static_cast<unsigned char>(buffer[length]) & 0xC0) == 0x80) --length;
        buffer[length] = 0;
      }
      strcat(buffer, "...");
    }
    u8g2_->drawUTF8(x, y + line * 16, buffer);
    cursor += length; while (*cursor == ' ') ++cursor;
  }
}
void NekoUi::card(int x, int y, int width, const char *title, const char *value) {
  u8g2_->drawRFrame(x, y, width, 44, 6);
  u8g2_->setFont(u8g2_font_5x8_tf); u8g2_->drawUTF8(x + 10, y + 12, title);
  u8g2_->setFont(u8g2_font_9x18B_tf); u8g2_->drawUTF8(x + 10, y + 33, value);
}
void NekoUi::row(int y, const char *label, const char *value) {
  u8g2_->setFont(u8g2_font_6x13_tf); u8g2_->drawUTF8(20, y, label);
  u8g2_->setFont(u8g2_font_7x14B_tf); wrapped(151, y, 225, value, 1);
  u8g2_->drawHLine(20, y + 10, 360);
}
void NekoUi::drawPet(int x, int y, int size, CatMood mood) {
  NekoPet::Pose pose = NekoPet::Pose::Idle;
  if (mood == CatMood::Happy) pose = NekoPet::Pose::Happy;
  else if (mood == CatMood::Sleepy || mood == CatMood::Offline || mood == CatMood::LowBattery) pose = NekoPet::Pose::Sleepy;
  else if (mood == CatMood::Alarm || mood == CatMood::Timer || mood == CatMood::Alert || mood == CatMood::Error || mood == CatMood::Hot || mood == CatMood::Humid)
    pose = NekoPet::Pose::Alert;
  const auto animation = NekoPet::sample(pose, millis());
  const uint8_t *base = size == 224 ? NekoPetAssets::Frame224_0 :
    size == 192 ? NekoPetAssets::Frame192_0 :
    size == 64 ? NekoPetAssets::Frame64_0 : NekoPetAssets::Frame160_0;
  NekoPet::raster(base,size,animation,petBuffer_.data());
  u8g2_->drawXBMP(x, y + animation.bob, size, size, petBuffer_.data());
}
void NekoUi::speech(int mouthX, int mouthY, const char *message, const char *author) {
  bool large=true;
  const auto quoteMeasure=[&](const char *text){u8g2_->setFont(large?u8g2_font_helvB18_tf:u8g2_font_helvB14_tf);return u8g2_->getUTF8Width(text);};
  const auto authorMeasure=[&](const char *text){u8g2_->setFont(u8g2_font_helvR10_tf);return u8g2_->getUTF8Width(text);};
  auto box=NekoSpeech::layout(message,author,quoteMeasure,authorMeasure);
  if(!box.fits){large=false;box=NekoSpeech::layout(message,author,quoteMeasure,authorMeasure,false);}
  const int x=box.x,y=box.y,width=box.width,height=box.height;
  u8g2_->drawRFrame(x,y,width,height,10);u8g2_->drawRFrame(x+1,y+1,width-2,height-2,9);
  const int join=constrain(mouthY-12,y+14,y+height-15);
  u8g2_->setDrawColor(1);
  u8g2_->drawTriangle(x,join-7,mouthX,mouthY,x,join+7);
  u8g2_->drawVLine(x,join-6,13);u8g2_->drawVLine(x+1,join-6,13);
  u8g2_->setDrawColor(0);
  u8g2_->drawLine(x,join-7,mouthX,mouthY);u8g2_->drawLine(mouthX,mouthY,x,join+7);
  u8g2_->setFont(large?u8g2_font_helvB18_tf:u8g2_font_helvB14_tf);
  for(unsigned i=0;i<box.textCount;++i)u8g2_->drawUTF8(x+14,box.textBaseline+i*box.spacing,box.text[i].text);
  u8g2_->setFont(u8g2_font_helvR10_tf);
  for(unsigned i=0;i<box.authorCount;++i)u8g2_->drawUTF8(x+14,box.authorBaseline+i*17,box.author[i].text);
}
void NekoUi::drawHome(CatMood mood, const char *message, const Settings &,
                      const NekoHardware &, bool, bool, bool,
                      const NekoPhrases::Entry *phrase) {
  u8g2_->drawXBMP(12,16,160,40,NekoBrandAssets::Logo);
  drawPet(0, 100, 192, mood);
  speech(175, 203, message, phrase ? phrase->author : nullptr);
}
void NekoUi::render(Page page, CatMood mood, const char *message, const Settings &settings,
                    const NekoHardware &hardware, const NekoScheduler &scheduler,
                    bool wifi, int8_t rssi, bool ble, bool timeValid) {
  if (!u8g2_) return;
  u8g2_->clearBuffer(); u8g2_->setDrawColor(1); u8g2_->drawBox(0, 0, 400, 300); u8g2_->setDrawColor(0);
  const bool urgent = mood == CatMood::Alarm || mood == CatMood::Timer || mood == CatMood::LowBattery || mood == CatMood::Error || mood == CatMood::Updating || mood == CatMood::Happy;
  const auto *phrase = !urgent && !scheduler.notification().active ? &NekoPhrases::current(millis()) : nullptr;
  if (page == Page::Home || page == Page::Cat) {
    drawHome(mood, phrase ? phrase->text : message, settings, hardware, wifi, ble, timeValid, phrase);
    flush(); return;
  }
  if (page == Page::Clock) { drawClock(timeValid, mood); flush(); return; }
  header(pageName(page), hardware, wifi, ble);
  char line[128]; const char *hint = "KEY/BOOT: menu  KEY+BOOT 2s: esci";
  u8g2_->setFont(u8g2_font_6x13_tf);
  switch (page) {
    case Page::DiagnosticsMenu:
      for (unsigned i=0;i<NekoNavigation::Count;++i) {
        const int y=63+i*24;
        if (i==diagnosticSelection_) {u8g2_->drawRBox(12,y-17,376,22,4);u8g2_->setDrawColor(1);}
        u8g2_->setFont(u8g2_font_7x14B_tf);u8g2_->drawUTF8(24,y,NekoNavigation::Labels[i]);
        u8g2_->setDrawColor(0);
      }
      hint="KEY/BOOT: scegli  2x: apri  KEY+BOOT: esci";break;
    case Page::Environment:
      if (hardware.environment().valid) {
        snprintf(line, sizeof(line), "%.1f C", hardware.environment().temperatureC); card(16, 57, 368, "TEMPERATURA", line);
        snprintf(line, sizeof(line), "%.0f %% RH", hardware.environment().humidityRh); card(16, 114, 368, "UMIDITA'", line);
        row(194, "Sensore", "SHTC3 / lettura ogni 60s");
        row(234, "Comfort", hardware.environment().temperatureC >= 29 ? "Fa caldo" : hardware.environment().humidityRh >= 70 ? "Molto umido" : "Tutto tranquillo");
      } else { centered(119, "Sensore non disponibile"); centered(149, "Controlla la pagina diagnostica"); } break;
    case Page::Timers: {
      int y = 63; bool any = false;
      for (const auto &timer : scheduler.timers()) if (timer.active) {
        snprintf(line, sizeof(line), "#%lu  %s", (unsigned long)timer.id, timer.label);
        u8g2_->setFont(u8g2_font_6x13B_tf); wrapped(22, y, 224, line, 1);
        snprintf(line, sizeof(line), "%02lu:%02lu%s", (unsigned long)(timer.remainingSeconds / 60),
          (unsigned long)(timer.remainingSeconds % 60), timer.paused ? " ||" : "");
        u8g2_->setFont(u8g2_font_9x18B_tf); u8g2_->drawStr(265, y, line); u8g2_->drawHLine(20, y + 13, 360);
        y += 52; any = true;
      }
      if (!any) { u8g2_->setFont(u8g2_font_9x18B_tf); centered(122, "Un momento per te.");
        u8g2_->setFont(u8g2_font_6x13_tf); centered(151, "Doppio click: avvia 5 minuti"); }
      hint = "2x: pausa/riprendi primo timer"; break;
    }
    case Page::Alarms: {
      int y = 57; bool any = false;
      for (const auto &alarm : scheduler.alarms()) if (alarm.enabled) {
        snprintf(line, sizeof(line), "%02u:%02u  %s", alarm.hour, alarm.minute, alarm.label);
        u8g2_->setFont(u8g2_font_7x14B_tf); wrapped(22, y, 350, line, 1); y += 25; any = true;
      }
      if (!any) { centered(122, "Nessuna sveglia attiva"); centered(149, "Configura fino a 8 sveglie dal companion"); } break;
    }
    case Page::Reminders: {
      int y = 61; bool any = false;
      for (const auto &item : scheduler.reminders()) if (item.active) {
        time_t due = item.dueEpoch; tm date{}; localtime_r(&due, &date);
        snprintf(line, sizeof(line), "%02d:%02d  %s", date.tm_hour, date.tm_min, item.text);
        wrapped(22, y, 352, line); y += 51; any = true;
      }
      if (!any) { centered(119, "Niente da ricordare, per ora."); centered(150, "Aggiungi un promemoria dal companion"); }
      hint = "2x: ricordami una pausa tra 10 minuti"; break;
    }
    case Page::Notifications:
      if (scheduler.notification().active) {
        u8g2_->drawRFrame(16, 70, 368, 130, 8); u8g2_->setFont(u8g2_font_9x18B_tf);
        wrapped(29, 103, 340, scheduler.notification().text, 5);
      } else { centered(129, "Tutto letto. Nessun messaggio."); }
      hint = "2x: segna il messaggio come letto"; break;
    case Page::Connectivity:
      row(65, "Wi-Fi / IP", wifi ? WiFi.localIP().toString().c_str() : "Non connesso");
      snprintf(line, sizeof(line), "%d dBm / BLE %s", rssi, ble ? "ON" : "pronto"); row(101, "Segnale", line); break;
    case Page::Battery: {
      snprintf(line, sizeof(line), "%u%%", hardware.battery().percent); u8g2_->setFont(u8g2_font_logisoso62_tn); centered(132, line);
      u8g2_->drawRFrame(48, 153, 304, 19, 4); const int fill = hardware.battery().percent * 298 / 100;
      if (fill) u8g2_->drawBox(51, 156, fill, 13);
      snprintf(line, sizeof(line), "%.3f V", hardware.battery().voltage); row(207, "Tensione", line);
      row(249, "Ricarica", hardware.battery().chargingEstimate ? "Tendenza positiva" : "Stima da tensione"); break;
    }
    case Page::Settings: {
      const char *profiles[] = {"Prestazioni", "Bilanciato", "Risparmio", "Notte", "Critico"};
      row(69, "Nome", settings.catName); row(112, "Profilo", profiles[min<unsigned>(static_cast<unsigned>(settings.powerProfile), 4)]);
      row(155, "Animazione", settings.powerProfile == PowerProfile::Night ? "Lenta / notte" : "Dither / occhi / coda");
      u8g2_->setFont(u8g2_font_6x13_tf); wrapped(20, 205, 352, "Nome, ora, timer e promemoria si modificano dal companion Bluetooth.", 3);
      hint = "2x: cambia profilo energetico"; break;
    }
    case Page::Storage:
      row(83, "Micro SD", hardware.sdOk() ? "Collegata" : "Assente");
      if (hardware.sdOk()) { snprintf(line, sizeof(line), "%.1f GB", SD_MMC.cardSize() / 1073741824.0); row(132, "Capacita'", line); }
      u8g2_->setFont(u8g2_font_6x13_tf); wrapped(20, 193, 352, "Aggiornamento da SD: neko-update.bin + neko-update.ready al riavvio.", 3); break;
    case Page::Audio:
      row(85, "Codec", hardware.audioCodecPresent() ? "ES8311 / ES7210 presenti" : "Non rilevati");
      u8g2_->setFont(u8g2_font_7x14B_tf); centered(147, "Avvisi visivi sul display");
      u8g2_->setFont(u8g2_font_6x13_tf); centered(181, "Audio non attivo in questo firmware"); break;
    case Page::Diagnostics:
      snprintf(line, sizeof(line), "%u B / %u B", ESP.getFreeHeap(), ESP.getFreePsram()); row(65, "RAM / PSRAM", line);
      row(101, "RTC", hardware.rtcOk() ? "OK" : "Non disponibile"); row(137, "Ambiente", hardware.environment().valid ? "OK" : "Errore");
      snprintf(line, sizeof(line), "%lu", (unsigned long)hardware.i2cErrors()); row(173, "Errori I2C", line);
      snprintf(line, sizeof(line), "%lu s", millis() / 1000UL); row(209, "Acceso da", line);
      snprintf(line, sizeof(line), "KEY=%d BOOT=%d", digitalRead(Board::KEY), digitalRead(Board::BOOT_KEY)); row(245, "Pulsanti", line); break;
    default: break;
  }
  footer(hint); if (page != Page::Connectivity) flush();
}

void NekoUi::drawClock(bool timeValid, CatMood) {
  const time_t now=time(nullptr); tm date{};localtime_r(&now,&date);
  NekoClock::draw(date.tm_hour,date.tm_min,timeValid,
    [this](int x,int y,int digit,int width,int height){u8g2_->drawXBMP(x,y,width,height,NekoClockAssets::Large[digit]);});
  u8g2_->drawBox(NekoClock::ColonLeft,120,8,10);u8g2_->drawBox(NekoClock::ColonLeft,173,8,10);
}

void NekoUi::flush() {
  lastTransferredTiles_ = NekoDisplayDiff::update(u8g2_->getBufferPtr(), previous_.data(),
    u8g2_->getBufferTileWidth(), u8g2_->getBufferTileHeight(), firstFrame_,
    [this](unsigned x, unsigned y, unsigned width) { u8g2_->updateDisplayArea(x, y, width, 1); });
  firstFrame_ = false;
}
void NekoUi::dumpScreen() {
  if (!u8g2_) return;
  const size_t size = u8g2_->getBufferTileWidth() * u8g2_->getBufferTileHeight() * 8;
  Serial.printf("NEKO_FRAME %u %u R1\n", u8g2_->getBufferTileWidth() * 8, u8g2_->getBufferTileHeight() * 8);
  const uint8_t *data = u8g2_->getBufferPtr();
  for (size_t offset = 0; offset < size; offset += 64) {
    char text[129]{}; const size_t count = min<size_t>(64, size - offset);
    for (size_t i = 0; i < count; ++i) snprintf(text + i * 2, 3, "%02x", data[offset + i]);
    Serial.println(text);
  }
  Serial.println("NEKO_END");
}

void NekoUi::connectionDetails(const char *ap, const char *password, bool setupOpen) {
  if (!u8g2_) return;
  row(144, "Login web", "utente: neko");
  row(180, "Password web", password);
  row(216, "Rete setup", setupOpen ? ap : "Tieni KEY / rilascia");
  row(252, "Browser", WiFi.status() == WL_CONNECTED ? "http://neko.local" : "http://192.168.4.1");
  flush();
}
