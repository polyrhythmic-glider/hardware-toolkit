#include "ui_manager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace neko {
static constexpr const char *kPageTitles[] = {
    "HOME", "OROLOGIO", "AMBIENTE", "TIMER", "SVEGLIE", "PROMEMORIA",
    "BATTERIA", "CONNESSIONI", "AUDIO", "IMPOSTAZIONI", "SISTEMA",
    "DIAGNOSTICA", "NEKO", "NOTTE", "AGGIORNAMENTO"
};

void UiManager::next_page() {
    page_ = static_cast<Page>((static_cast<uint8_t>(page_) + 1) % static_cast<uint8_t>(Page::Count));
}

void UiManager::previous_page() {
    const uint8_t count = static_cast<uint8_t>(Page::Count);
    page_ = static_cast<Page>((static_cast<uint8_t>(page_) + count - 1) % count);
}

void UiManager::set_page(Page page) {
    if (page < Page::Count) page_ = page;
}

void UiManager::draw_radio_icons(u8g2_t *u, int x, int y) {
    // Wi-Fi: three monochrome arcs plus a dot; strike when disconnected.
    u8g2_DrawArc(u, x + 10, y + 10, 9, 32, 96);
    u8g2_DrawArc(u, x + 10, y + 10, 6, 32, 96);
    u8g2_DrawDisc(u, x + 10, y + 10, 1);
    if (!model_.wifi_connected) u8g2_DrawLine(u, x + 1, y + 1, x + 18, y + 14);
    x += 27;
    // Bluetooth rune.
    u8g2_DrawLine(u, x + 7, y, x + 7, y + 15);
    u8g2_DrawLine(u, x + 7, y, x + 13, y + 5);
    u8g2_DrawLine(u, x + 13, y + 5, x + 3, y + 12);
    u8g2_DrawLine(u, x + 7, y + 15, x + 13, y + 10);
    u8g2_DrawLine(u, x + 13, y + 10, x + 3, y + 3);
    if (!model_.ble_connected) u8g2_DrawLine(u, x, y, x + 15, y + 15);
}

void UiManager::draw_battery_icon(u8g2_t *u, int x, int y) {
    u8g2_DrawFrame(u, x, y, 30, 14);
    u8g2_DrawBox(u, x + 30, y + 4, 3, 6);
    const int fill = std::clamp<int>((model_.battery.percent * 26) / 100, 0, 26);
    if (fill) u8g2_DrawBox(u, x + 2, y + 2, fill, 10);
    if (model_.battery.charging_estimate) {
        u8g2_SetDrawColor(u, 2);
        u8g2_DrawTriangle(u, x + 17, y + 1, x + 11, y + 8, x + 16, y + 8);
        u8g2_DrawTriangle(u, x + 14, y + 6, x + 20, y + 6, x + 14, y + 13);
        u8g2_SetDrawColor(u, 1);
    }
}

void UiManager::draw_header(u8g2_t *u, const char *title) {
    u8g2_SetFont(u, u8g2_font_6x13_tf);
    u8g2_DrawStr(u, 8, 15, title);
    draw_radio_icons(u, 280, 3);
    draw_battery_icon(u, 337, 3);
    char percent[8]; std::snprintf(percent, sizeof(percent), "%u", model_.battery.percent);
    u8g2_DrawStr(u, 374, 15, percent);
    u8g2_DrawHLine(u, 0, 22, 400);
}

void UiManager::draw_home(u8g2_t *u) {
    draw_header(u, model_.cat_name);
    tm local{}; localtime_r(&model_.now, &local);
    char time_text[8]; std::strftime(time_text, sizeof(time_text), "%H:%M", &local);
    u8g2_SetFont(u, u8g2_font_logisoso50_tn);
    u8g2_DrawStr(u, 14, 88, time_text);
    char date[48]; std::strftime(date, sizeof(date), "%a %d %b %Y", &local);
    u8g2_SetFont(u, u8g2_font_9x18B_tf);
    u8g2_DrawStr(u, 18, 116, date);
    u8g2_SetFont(u, u8g2_font_6x13_tf);
    const char *source = model_.time.source == TimeSource::Network ? "ORA: RETE" :
                         model_.time.source == TimeSource::HardwareRtc ? "ORA: RTC" : "ORA: NON AFFIDABILE";
    u8g2_DrawStr(u, 20, 138, source);

    u8g2_DrawRFrame(u, 10, 154, 224, 62, 5);
    char env[48];
    if (model_.environment.valid) {
        std::snprintf(env, sizeof(env), "INTERNO  %.1f C    %.0f %%RH",
                      model_.environment.temperature_c, model_.environment.humidity_rh);
    } else {
        std::snprintf(env, sizeof(env), "SENSORE AMBIENTE NON DISPONIBILE");
    }
    u8g2_DrawStr(u, 20, 175, env);
    u8g2_DrawStr(u, 20, 198, model_.next_event);

    CatAvatar::draw(u, 315, 42, 2, model_.cat.expression, animation_frame_++);
    u8g2_DrawRFrame(u, 10, 228, 380, 58, 5);
    u8g2_DrawStr(u, 22, 249, model_.status_line);
    u8g2_SetFont(u, u8g2_font_5x8_tf);
    u8g2_DrawStr(u, 22, 272, "KEY pagina  |  doppio azione rapida  |  lungo menu");
}

static void value_line(u8g2_t *u, int y, const char *label, const char *value) {
    u8g2_SetFont(u, u8g2_font_6x13_tf);
    u8g2_DrawStr(u, 20, y, label);
    u8g2_SetFont(u, u8g2_font_9x18B_tf);
    u8g2_DrawStr(u, 175, y, value);
    u8g2_DrawHLine(u, 20, y + 7, 360);
}

void UiManager::draw_page(u8g2_t *u) {
    draw_header(u, kPageTitles[static_cast<uint8_t>(page_)]);
    char a[64]{}; char b[64]{};
    tm local{}; localtime_r(&model_.now, &local);
    switch (page_) {
    case Page::Clock:
        std::strftime(a, sizeof(a), "%H:%M", &local);
        u8g2_SetFont(u, u8g2_font_logisoso50_tn); u8g2_DrawStr(u, 85, 112, a);
        std::strftime(a, sizeof(a), "%A %d/%m/%Y", &local);
        u8g2_SetFont(u, u8g2_font_9x18B_tf); u8g2_DrawStr(u, 80, 150, a);
        value_line(u, 205, "SORGENTE", model_.time.valid ?
            (model_.time.source == TimeSource::Network ? "SNTP + RTC" : "RTC OFFLINE") : "NON AFFIDABILE");
        break;
    case Page::Environment:
        if (model_.environment.valid) {
            std::snprintf(a, sizeof(a), "%.1f C", model_.environment.temperature_c);
            std::snprintf(b, sizeof(b), "%.0f %%RH", model_.environment.humidity_rh);
        } else { std::strcpy(a, "--"); std::strcpy(b, "--"); }
        value_line(u, 72, "TEMPERATURA", a); value_line(u, 122, "UMIDITA", b);
        value_line(u, 172, "TIPO", "AMBIENTE LOCALE");
        u8g2_SetFont(u, u8g2_font_6x13_tf); u8g2_DrawStr(u, 20, 230, "Compensazione termica scheda applicata.");
        break;
    case Page::Timers:
        value_line(u, 75, "TIMER ATTIVI", model_.next_event);
        value_line(u, 125, "CONTROLLO", "KEY / BLE");
        u8g2_DrawStr(u, 20, 185, "Pausa, ripresa e timer multipli sono persistenti.");
        break;
    case Page::Alarms:
        value_line(u, 75, "PROSSIMA", model_.next_event); value_line(u, 125, "SNOOZE", "9 MIN");
        u8g2_DrawStr(u, 20, 185, "Ricorrenze settimanali; funzionamento offline.");
        break;
    case Page::Reminders:
        value_line(u, 75, "PROSSIMO", model_.next_event);
        u8g2_DrawStr(u, 20, 135, "Testo breve configurabile dalla companion BLE.");
        break;
    case Page::Battery:
        std::snprintf(a, sizeof(a), "%u %% (STEP 5%%)", model_.battery.percent);
        std::snprintf(b, sizeof(b), "%.3f V", model_.battery.voltage_v);
        value_line(u, 65, "STIMA", a); value_line(u, 105, "TENSIONE", b);
        std::snprintf(a, sizeof(a), "%d / %d mV", model_.battery.raw, model_.battery.adc_mv);
        value_line(u, 145, "RAW / ADC", a);
        value_line(u, 185, "CALIBRAZIONE", model_.battery.calibrated ? "EFUSE" : "APPROSSIMATA");
        value_line(u, 225, "RICARICA", model_.battery.charging_estimate ? "STIMA DA TREND" : "NON RILEVATA");
        break;
    case Page::Connections:
        value_line(u, 65, "WI-FI", model_.wifi_connected ? "CONNESSO" : "OFFLINE");
        value_line(u, 105, "BLUETOOTH LE", model_.ble_connected ? "CONNESSO" : "ADVERTISING");
        std::snprintf(a, sizeof(a), "%d dBm", model_.wifi_rssi); value_line(u, 145, "RSSI", a);
        value_line(u, 185, "IP", model_.ip[0] ? model_.ip : "--");
        value_line(u, 225, "SICUREZZA BLE", "PAIRING + BONDING");
        break;
    case Page::Audio:
        std::snprintf(a, sizeof(a), "%u %%", model_.volume); value_line(u, 70, "VOLUME", a);
        value_line(u, 115, "MUTE", model_.muted ? "SI" : "NO");
        value_line(u, 160, "USCITA", "ES8311 + NS4150B");
        value_line(u, 205, "INGRESSO", "ES7210, 2 MIC");
        break;
    case Page::Settings:
        value_line(u, 65, "NOME", model_.cat_name); value_line(u, 105, "TIMEZONE", "EUROPE/ROME");
        value_line(u, 145, "PROFILO", "BILANCIATO"); value_line(u, 185, "LINGUA", "ITALIANO");
        u8g2_DrawStr(u, 20, 240, "Modifica completa via BLE; factory reset: KEY >10 s al boot.");
        break;
    case Page::SystemInfo:
        value_line(u, 65, "FIRMWARE", "NEKO 0.1.0"); value_line(u, 105, "HARDWARE", "WS_RLCD42_R1");
        std::snprintf(a, sizeof(a), "%lu s", static_cast<unsigned long>(model_.uptime_seconds));
        value_line(u, 145, "UPTIME", a);
        std::snprintf(a, sizeof(a), "%lu / %lu", static_cast<unsigned long>(model_.free_heap),
                      static_cast<unsigned long>(model_.free_psram));
        value_line(u, 185, "HEAP / PSRAM", a);
        value_line(u, 225, "DISPLAY", "ST7305 RLCD 400x300");
        break;
    case Page::Diagnostics:
        std::snprintf(a, sizeof(a), "%lu", static_cast<unsigned long>(model_.i2c_errors));
        std::snprintf(b, sizeof(b), "%lu", static_cast<unsigned long>(model_.spi_errors));
        value_line(u, 60, "ERRORI I2C", a); value_line(u, 95, "ERRORI SPI", b);
        value_line(u, 130, "RTC", model_.time.valid ? "OK" : "NON VALIDO");
        value_line(u, 165, "SHTC3", model_.environment.valid ? "OK" : "ERRORE");
        value_line(u, 200, "MICROSD", model_.storage.mounted ? "MONTATA" : "ASSENTE");
        value_line(u, 235, "BATTERIA", model_.battery.valid ? "OK" : "ERRORE");
        break;
    case Page::CatFullscreen:
        CatAvatar::draw(u, 200, 35, 3, model_.cat.expression, animation_frame_++);
        u8g2_SetFont(u, u8g2_font_9x18B_tf); u8g2_DrawStr(u, 30, 280, model_.status_line);
        break;
    case Page::Night:
        u8g2_DrawFrame(u, 20, 45, 360, 215);
        std::strftime(a, sizeof(a), "%H:%M", &local);
        u8g2_SetFont(u, u8g2_font_logisoso50_tn); u8g2_DrawStr(u, 110, 135, a);
        u8g2_SetFont(u, u8g2_font_9x18B_tf); u8g2_DrawStr(u, 120, 195, model_.next_event);
        break;
    case Page::FirmwareUpdate:
        value_line(u, 75, "STATO", "IN ATTESA"); value_line(u, 125, "CANALI", "HTTPS / MICROSD");
        value_line(u, 175, "PROTEZIONE", "ROLLBACK ATTIVO");
        u8g2_DrawStr(u, 20, 235, "Richiede batteria >=50% o alimentazione stabile.");
        break;
    case Page::Home:
    case Page::Count: break;
    }
    u8g2_SetFont(u, u8g2_font_5x8_tf);
    u8g2_DrawStr(u, 16, 291, "KEY: pagina successiva    KEY lungo: menu");
}

void UiManager::render(bool full_refresh) {
    DisplayGuard guard(display_);
    if (!guard.held()) return;
    const bool page_changed = rendered_page_ != page_;
    display_.clear();
    u8g2_t *u = display_.canvas();
    u8g2_SetDrawColor(u, 1);
    if (page_ == Page::Home) draw_home(u); else draw_page(u);
    if (full_refresh || page_changed) display_.flush();
    else if (page_ == Page::Home || page_ == Page::Clock || page_ == Page::Night) {
        // ST7305 is an RLCD, not e-paper. The official tile driver supports
        // regional writes; normal updates only touch the logical page region.
        display_.flush_area(0, 24, 250, 130);
    } else {
        display_.flush();
    }
    rendered_page_ = page_;
}

}  // namespace neko
