#include "NekoUi.h"
#include "NekoCatSprites.h"

#include <SD_MMC.h>
#include <WiFi.h>

bool NekoUi::begin() {
  lcd_.begin(0, U8G2_R1);
  u8g2_ = lcd_.getU8g2();
  if (u8g2_) {
    // I font e le XBM devono modificare solo i pixel accesi della maschera.
    u8g2_->setFontMode(1);
    u8g2_->setBitmapMode(1);
  }
  return u8g2_ != nullptr;
}

void NekoUi::powerSave(bool enabled) {
  if (u8g2_) u8g2_->setPowerSave(enabled ? 1 : 0);
}

void NekoUi::centered(int y, const char *text) {
  const int x = max(0, (400 - u8g2_->getUTF8Width(text)) / 2);
  u8g2_->drawUTF8(x, y, text);
}

const char *NekoUi::pageName(Page page) {
  static const char *names[] = {"HOME", "OROLOGIO", "TIMER", "SVEGLIE", "PROMEMORIA",
    "METEO", "AMBIENTE", "NOTIFICHE", "AUDIO", "MEMORIA", "CONNETTIVITÀ",
    "BATTERIA", "IMPOSTAZIONI", "DIAGNOSTICA", "INFO"};
  return names[static_cast<uint8_t>(page)];
}

const char *NekoUi::moodName(CatMood mood) {
  static const char *names[] = {"Tranquillo", "Felice", "Assonnato", "Attento", "In ascolto",
    "Pensa...", "Parla", "Che caldo", "Troppa umidita", "Batteria bassa", "Offline",
    "Timer", "Sveglia", "Errore", "Aggiornamento"};
  return names[static_cast<uint8_t>(mood)];
}

void NekoUi::header(const char *title, const NekoHardware &hardware, bool wifi, bool ble) {
  u8g2_->setFont(u8g2_font_6x13B_tf);
  u8g2_->drawUTF8(8, 15, title);
  char status[64];
  snprintf(status, sizeof(status), "%s %s  %u%%", wifi ? "W" : "-", ble ? "B" : "-",
           hardware.battery().percent);
  u8g2_->drawUTF8(400 - u8g2_->getUTF8Width(status) - 8, 15, status);
  u8g2_->drawHLine(6, 20, 388);
}

void NekoUi::footer(const char *hint) {
  u8g2_->drawHLine(6, 278, 388);
  u8g2_->setFont(u8g2_font_5x8_tf);
  u8g2_->drawUTF8(8, 292, hint);
}

void NekoUi::drawCat(int cx, int cy, int scale, CatMood mood) {
  (void)scale;  // sprite nativa: nessun ridimensionamento/dithering sul pannello 1-bit
  const uint8_t *sprite = NEKO_CAT_IDLE;
  switch (mood) {
    case CatMood::Happy: sprite = NEKO_CAT_HAPPY; break;
    case CatMood::Sleepy:
    case CatMood::Offline:
    case CatMood::LowBattery: sprite = NEKO_CAT_SLEEPY; break;
    case CatMood::Alert:
    case CatMood::Listening:
    case CatMood::Speaking:
    case CatMood::Timer:
    case CatMood::Alarm:
    case CatMood::Error:
    case CatMood::Updating: sprite = NEKO_CAT_ALERT; break;
    default: break;
  }
  u8g2_->drawXBMP(cx - NEKO_SPRITE_WIDTH / 2, cy - NEKO_SPRITE_HEIGHT / 2,
                  NEKO_SPRITE_WIDTH, NEKO_SPRITE_HEIGHT, sprite);

  // Piccoli indicatori contestuali restano fuori dalla sprite per non alterare
  // la silhouette principale e conservare quattro frame riutilizzabili.
  if (mood == CatMood::Hot) {
    for (int x = cx + 56; x <= cx + 68; x += 6) {
      u8g2_->drawLine(x, cy - 22, x + 2, cy - 16);
      u8g2_->drawLine(x + 2, cy - 16, x, cy - 10);
    }
  } else if (mood == CatMood::Humid) {
    u8g2_->drawTriangle(cx + 62, cy - 26, cx + 54, cy - 10, cx + 70, cy - 10);
    u8g2_->drawCircle(cx + 62, cy - 9, 8, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
  } else if (mood == CatMood::LowBattery) {
    u8g2_->drawFrame(cx + 54, cy - 24, 20, 11);
    u8g2_->drawBox(cx + 74, cy - 21, 3, 5);
    u8g2_->drawBox(cx + 57, cy - 21, 4, 5);
  }
}

void NekoUi::drawHome(CatMood mood, const char *message, const Settings &settings,
                      const NekoHardware &hardware, bool wifi, bool ble, bool timeValid) {
  header(settings.catName, hardware, wifi, ble);
  drawCat(200, 112, 2, mood);
  u8g2_->setFont(u8g2_font_7x14B_tf);
  centered(204, message && *message ? message : moodName(mood));
  char line[96];
  if (hardware.environment().valid) {
    snprintf(line, sizeof(line), "%.1f C   %.0f%% RH", hardware.environment().temperatureC,
             hardware.environment().humidityRh);
  } else strlcpy(line, "SHTC3 non disponibile", sizeof(line));
  u8g2_->setFont(u8g2_font_6x13_tf);
  centered(228, line);
  if (timeValid) {
    tm local{}; const time_t now = time(nullptr); localtime_r(&now, &local);
    snprintf(line, sizeof(line), "%02d:%02d  %02d/%02d/%04d", local.tm_hour, local.tm_min,
             local.tm_mday, local.tm_mon + 1, local.tm_year + 1900);
  } else strlcpy(line, "Ora in attesa di RTC/SNTP", sizeof(line));
  centered(250, line);
  footer("click: pagina   doppio: timer 5m   lungo: BLE");
}

void NekoUi::render(Page page, CatMood mood, const char *message, const Settings &settings,
                    const NekoHardware &hardware, const NekoScheduler &scheduler,
                    bool wifi, int8_t rssi, bool ble, bool timeValid) {
  if (!u8g2_) return;
  u8g2_->clearBuffer();
  // Sul mapping ST7305 della scheda, un buffer pieno produce il bianco.
  // Tutti gli elementi vengono poi sottratti in nero con draw color 0.
  u8g2_->setDrawColor(1);
  u8g2_->drawBox(0, 0, 400, 300);
  u8g2_->setDrawColor(0);
  if (page == Page::Home) {
    drawHome(mood, message, settings, hardware, wifi, ble, timeValid);
    u8g2_->sendBuffer();
    return;
  }
  header(pageName(page), hardware, wifi, ble);
  u8g2_->setFont(u8g2_font_6x13_tf);
  char line[128];
  switch (page) {
    case Page::Clock: {
      tm local{}; const time_t now = time(nullptr); localtime_r(&now, &local);
      snprintf(line, sizeof(line), "%02d:%02d", local.tm_hour, local.tm_min);
      u8g2_->setFont(u8g2_font_logisoso46_tn); centered(118, line);
      u8g2_->setFont(u8g2_font_7x14_tf);
      snprintf(line, sizeof(line), "%02d/%02d/%04d", local.tm_mday, local.tm_mon + 1, local.tm_year + 1900);
      centered(154, line); break;
    }
    case Page::Timers: {
      int y = 50; bool any = false;
      for (const auto &timer : scheduler.timers()) if (timer.active) {
        snprintf(line, sizeof(line), "#%lu %-18s %02lu:%02lu %s", (unsigned long)timer.id, timer.label,
          (unsigned long)(timer.remainingSeconds / 60), (unsigned long)(timer.remainingSeconds % 60), timer.paused ? "PAUSA" : "");
        u8g2_->drawUTF8(15, y, line); y += 25; any = true;
      }
      if (!any) centered(130, "Nessun timer attivo"); break;
    }
    case Page::Alarms: {
      int y = 48; bool any = false;
      for (const auto &alarm : scheduler.alarms()) if (alarm.enabled) {
        snprintf(line, sizeof(line), "%02u:%02u  %-18s  giorni:%02X", alarm.hour, alarm.minute, alarm.label, alarm.weekdays);
        u8g2_->drawUTF8(15, y, line); y += 24; any = true;
      }
      if (!any) centered(130, "Nessuna sveglia configurata"); break;
    }
    case Page::Reminders: centered(105, "Promemoria gestibili via BLE"); centered(135, "e companion Web Bluetooth"); break;
    case Page::Weather: centered(105, wifi ? "Online, provider meteo non configurato" : "Wi-Fi offline"); centered(140, "La UI non inventa dati cloud"); break;
    case Page::Environment:
      snprintf(line, sizeof(line), "Temperatura: %.2f C", hardware.environment().temperatureC); u8g2_->drawUTF8(35, 80, line);
      snprintf(line, sizeof(line), "Umidità:     %.1f %%RH", hardware.environment().humidityRh); u8g2_->drawUTF8(35, 115, line);
      snprintf(line, sizeof(line), "SHTC3:       %s", hardware.environment().valid ? "OK" : "ERRORE"); u8g2_->drawUTF8(35, 150, line); break;
    case Page::Notifications:
      if (scheduler.notification().active) { u8g2_->drawRFrame(15, 55, 370, 100, 8); u8g2_->drawUTF8(28, 90, scheduler.notification().text); }
      else centered(120, "Nessuna notifica"); break;
    case Page::Audio:
      centered(95, hardware.audioCodecPresent() ? "ES8311 + ES7210 rilevati" : "Codec audio non rilevati");
      centered(130, "Test audio completo: build ESP-IDF"); break;
    case Page::Storage:
      snprintf(line, sizeof(line), "microSD: %s", hardware.sdOk() ? "montata" : "non montata / assente"); centered(100, line);
      if (hardware.sdOk()) { snprintf(line, sizeof(line), "Capacità: %.1f GB", SD_MMC.cardSize() / 1073741824.0); centered(135, line); } break;
    case Page::Connectivity:
      snprintf(line, sizeof(line), "Wi-Fi: %s  RSSI: %d dBm", wifi ? WiFi.localIP().toString().c_str() : "offline", rssi); u8g2_->drawUTF8(25, 75, line);
      snprintf(line, sizeof(line), "BLE: %s", ble ? "connesso" : "in advertising"); u8g2_->drawUTF8(25, 110, line); break;
    case Page::Battery:
      snprintf(line, sizeof(line), "%u%%", hardware.battery().percent); u8g2_->setFont(u8g2_font_logisoso46_tn); centered(115, line);
      u8g2_->setFont(u8g2_font_6x13_tf); snprintf(line, sizeof(line), "%.3f V  %s", hardware.battery().voltage,
        hardware.battery().chargingEstimate ? "carica stimata" : "su batteria"); centered(150, line); break;
    case Page::Settings:
      snprintf(line, sizeof(line), "Nome: %s", settings.catName); u8g2_->drawUTF8(25, 65, line);
      snprintf(line, sizeof(line), "Timezone: %s", settings.timezone); u8g2_->drawUTF8(25, 95, line);
      snprintf(line, sizeof(line), "Volume: %u%%  Profilo: %u", settings.volume, static_cast<unsigned>(settings.powerProfile)); u8g2_->drawUTF8(25, 125, line); break;
    case Page::Diagnostics:
      snprintf(line, sizeof(line), "Heap libero: %u", ESP.getFreeHeap()); u8g2_->drawUTF8(20, 55, line);
      snprintf(line, sizeof(line), "PSRAM libera: %u", ESP.getFreePsram()); u8g2_->drawUTF8(20, 80, line);
      snprintf(line, sizeof(line), "RTC:%s SHTC3:%s SD:%s AUDIO:%s", hardware.rtcOk() ? "OK" : "--", hardware.environment().valid ? "OK" : "--",
        hardware.sdOk() ? "OK" : "--", hardware.audioCodecPresent() ? "OK" : "--"); u8g2_->drawUTF8(20, 105, line);
      snprintf(line, sizeof(line), "Errori I2C: %lu", (unsigned long)hardware.i2cErrors()); u8g2_->drawUTF8(20, 130, line);
      snprintf(line, sizeof(line), "KEY18:%d  BOOT0:%d", digitalRead(Board::KEY), digitalRead(Board::BOOT_KEY)); u8g2_->drawUTF8(20, 155, line); break;
    case Page::About:
      u8g2_->setFont(u8g2_font_7x14B_tf); centered(85, "NEKO ASSISTANT 0.2.0-arduino");
      u8g2_->setFont(u8g2_font_6x13_tf); centered(120, "Waveshare ESP32-S3-RLCD-4.2"); centered(150, "Arduino-ESP32 3.3.x"); break;
    default: break;
  }
  footer("click: pagina successiva   lungo: provisioning");
  u8g2_->sendBuffer();
}
