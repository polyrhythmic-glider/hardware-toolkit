#include "NekoApp.h"
#include <SD_MMC.h>
#include <Update.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <sys/time.h>

RTC_NOINIT_ATTR static uint32_t crashBootCount;
NekoApp::NekoApp() : connectivity_(settings_, hardware_, scheduler_), web_(settings_, hardware_, scheduler_, connectivity_) {}

void NekoApp::begin() {
  Serial.begin(115200); delay(200);
  Serial.println("\n[NEKO] Pixel Pet 0.7.0 boot");
  pinMode(Board::KEY, INPUT_PULLUP); pinMode(Board::BOOT_KEY, INPUT_PULLUP);
  pinMode(Board::PA_ENABLE, OUTPUT); digitalWrite(Board::PA_ENABLE, LOW);
  const auto reason = esp_reset_reason();
  const bool crashed = reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT;
  crashBootCount = crashed ? crashBootCount + 1 : 0;
  settings_.begin(); setenv("TZ", settings_.get().timezone, 1); tzset();
  hardware_.begin(settings_.get().sensorOffsetC); scheduler_.begin(); bootstrapTime();
  if (!ui_.begin()) Serial.println("[NEKO] display allocation failed");
  render(crashBootCount >= 3 ? "Safe mode. Radio disattivata." : "Ciao. Sono il tuo piccolo Neko.");
  hardware_.mountSd(); checkSdUpdate();
  if (crashBootCount < 3) { connectivity_.begin(); web_.begin(); }
  applyPowerProfile();
  Serial.printf("[NEKO] ready rtc=%d sensor=%d sd=%d battery=%u%%\n", hardware_.rtcOk(), hardware_.environment().valid, hardware_.sdOk(), hardware_.battery().percent);
  Serial.println("[NEKO] USB commands: help, status, home, cat, timer 300, pause, resume, cancel, wifi, screen");
  render();
}
void NekoApp::bootstrapTime() {
  tm rtc{}; if (!hardware_.readRtc(rtc)) return;
  const time_t epoch = mktime(&rtc); if (epoch < 1700000000) return;
  timeval tv{epoch, 0}; settimeofday(&tv, nullptr);
}
void NekoApp::applyPowerProfile() {
  appliedProfile_ = settings_.get().powerProfile;
  const unsigned cpu[] = {240, 160, 80, 80, 80};
  WiFi.setSleep(static_cast<unsigned>(appliedProfile_) >= static_cast<unsigned>(PowerProfile::Saver));
  setCpuFrequencyMhz(cpu[min<unsigned>(static_cast<unsigned>(appliedProfile_), 4)]);
}
void NekoApp::checkSdUpdate() {
  if (!hardware_.sdOk() || !SD_MMC.exists("/neko-update.ready") || !SD_MMC.exists("/neko-update.bin")) return;
  if (hardware_.battery().percent < 50 && !hardware_.battery().chargingEstimate) {
    scheduler_.pushNotification("Aggiornamento SD: serve almeno il 50% di batteria", 2, 300); return;
  }
  File firmware = SD_MMC.open("/neko-update.bin", FILE_READ);
  if (!firmware || !Update.begin(firmware.size(), U_FLASH)) return;
  mood_ = CatMood::Updating; render("Aggiornamento da microSD...");
  const size_t written = Update.writeStream(firmware);
  const bool ok = written == firmware.size() && Update.end(true); firmware.close();
  if (ok) { SD_MMC.remove("/neko-update.ready"); ESP.restart(); }
  Update.abort(); mood_ = CatMood::Error; render("Aggiornamento fallito. Riprova da USB.");
}
void NekoApp::pollButtons() {
  const uint32_t now=millis();const bool key=digitalRead(Board::KEY)==LOW,boot=digitalRead(Board::BOOT_KEY)==LOW;
  const bool wasSuppressed=chord_.suppress();
  if (chord_.update(key,boot,now)) {
    diagnosticOpen_=!diagnosticOpen_;diagnosticSelection_=0;
    page_=diagnosticOpen_?Page::DiagnosticsMenu:Page::Home;render();
  }
  if (wasSuppressed||chord_.suppress()) {key_=NekoButton{};boot_=NekoButton{};return;}
  const NekoButton::Event events[]={key_.update(key,now),boot_.update(boot,now)};
  for(unsigned button=0;button<2;++button){
    const auto event=events[button];if(event==NekoButton::Event::None)continue;
    if(event==NekoButton::Event::Click){
      if(diagnosticOpen_){
        if(page_==Page::DiagnosticsMenu) diagnosticSelection_=(diagnosticSelection_+(button?NekoNavigation::Count-1:1))%NekoNavigation::Count;
        page_=Page::DiagnosticsMenu;
      }else page_=NekoNavigation::next(page_);
      render();
    }else if(event==NekoButton::Event::Double)onDoublePress();
  }
}
void NekoApp::onDoublePress() {
  if(diagnosticOpen_&&page_==Page::DiagnosticsMenu){
    if(diagnosticSelection_==NekoNavigation::Count-1){diagnosticOpen_=false;page_=Page::Home;}
    else page_=NekoNavigation::DiagnosticPages[diagnosticSelection_];
    if(page_==Page::Connectivity){connectivity_.openProvisioningWindow(120);web_.openSetupWindow(120);}
    render();return;
  }
  if (page_ == Page::Clock) { NekoPhrases::next(); render(); return; }
  if (page_ == Page::Cat || page_ == Page::Home) { mood_ = CatMood::Happy; moodUntilMs_ = millis() + 7000; render("Purrr. Ancora una coccola?"); return; }
  if (page_ == Page::Settings) {
    Settings value = settings_.get(); value.powerProfile = static_cast<PowerProfile>((static_cast<unsigned>(value.powerProfile) + 1) % 4);
    settings_.save(value); applyPowerProfile(); render(); return;
  }
  if (page_ == Page::Notifications) { scheduler_.dismissNotification(); render(); return; }
  if (page_ == Page::Reminders) {
    const bool ok = scheduler_.addReminder(time(nullptr) + 600, "E' il momento di una pausa.");
    render(ok ? "Ti ricordo una pausa tra 10 minuti." : "Sincronizza l'ora o libera un promemoria."); return;
  }
  if (page_ == Page::Timers) for (const auto &timer : scheduler_.timers()) if (timer.active) {
    if (timer.paused) scheduler_.resumeTimer(timer.id); else scheduler_.pauseTimer(timer.id);
    render(); return;
  }
  if (scheduler_.startTimer(300, "Pausa breve")) {
    page_ = Page::Timers; mood_ = CatMood::Happy; moodUntilMs_ = millis() + 5000;
    render("Cinque minuti tutti per te.");
  } else render("Tutti e quattro i timer sono occupati.");
}
void NekoApp::updateMood() {
  char label[24];
  if (scheduler_.takeAlarmEvent(label, sizeof(label))) {
    mood_ = CatMood::Alarm; moodUntilMs_ = millis() + 60000;
    snprintf(message_, sizeof(message_), "Sveglia: %s", label); page_ = Page::Home; render(); return;
  }
  if (scheduler_.takeTimerEvent(label, sizeof(label))) {
    mood_ = CatMood::Timer; moodUntilMs_ = millis() + 30000;
    snprintf(message_, sizeof(message_), "Tempo scaduto: %s", label); page_ = Page::Home; render(); return;
  }
  // Alerts and petting keep their expression until expiry.
  if (moodUntilMs_ && static_cast<int32_t>(millis() - moodUntilMs_) < 0) return;
  const auto &notice = scheduler_.notification();
  if (notice.active && notice.expiresMs != lastNotification_) {
    lastNotification_ = notice.expiresMs; mood_ = CatMood::Alert; moodUntilMs_ = millis() + 10000;
    strlcpy(message_, notice.text, sizeof(message_)); render(); return;
  }
  CatMood next = CatMood::Idle;
  const auto &environment = hardware_.environment();
  if (hardware_.battery().critical) next = CatMood::LowBattery;
  else if (settings_.get().powerProfile == PowerProfile::Night) next = CatMood::Sleepy;
  else if (environment.valid && environment.temperatureC >= 29) next = CatMood::Hot;
  else if (environment.valid && environment.humidityRh >= 70) next = CatMood::Humid;
  else if (settings_.get().ssid[0] && !connectivity_.wifiConnected()) next = CatMood::Offline;
  if (next != mood_ || moodUntilMs_) {
    mood_ = next; moodUntilMs_ = 0;
    const char *text = next == CatMood::Sleepy ? "Shhh. Un piccolo riposo." : next == CatMood::Hot ? "Fa caldo. Una pausa all'ombra?" :
      next == CatMood::Humid ? "Aria umida. Apriamo una finestra?" : next == CatMood::LowBattery ? "Ho bisogno di un po' di energia." :
      next == CatMood::Offline ? "Wi-Fi in attesa. Io resto qui." : "Tutto tranquillo. Ti tengo compagnia.";
    strlcpy(message_, text, sizeof(message_)); render();
  }
}
void NekoApp::render(const char *message) {
  if (message) strlcpy(message_, message, sizeof(message_));
  ui_.setDiagnosticSelection(diagnosticSelection_);
  ui_.render(page_, mood_, message_, settings_.get(), hardware_, scheduler_, connectivity_.wifiConnected(), connectivity_.rssi(), connectivity_.bleConnected(), connectivity_.timeValid());
  if (page_ == Page::Connectivity) ui_.connectionDetails(web_.apName(), web_.password(), web_.setupOpen());
  lastRenderMs_ = millis();
}
void NekoApp::serialTick() {
  for (unsigned count = 0; count < 128 && Serial.available(); ++count) {
    const char c = Serial.read(); if (c == '\r') continue;
    if (c == '\n') {
      if (!serialOverflow_ && serialLength_) { serialLine_[serialLength_] = 0; serialCommand(serialLine_); }
      serialLength_ = 0; serialOverflow_ = false;
    } else if (serialLength_ < sizeof(serialLine_) - 1) serialLine_[serialLength_++] = c;
    else serialOverflow_ = true;
  }
}
void NekoApp::serialCommand(char *command) {
  if (!strncmp(command, "join\t", 5)) {
    char *password = strchr(command + 5, '\t');
    if (!password) { Serial.println("[WIFI] invalid configuration"); return; }
    *password++ = 0;
    const bool ok = connectivity_.configureWifi(command + 5, password);
    memset(password, 0, strlen(password));
    Serial.println(ok ? "[WIFI] configuration saved" : "[WIFI] configuration failed");
    return;
  }
  if (!strcmp(command, "web-key")) { Serial.printf("NEKO_WEB_KEY %s\n", web_.password()); return; }
  if (!strcmp(command, "screen")) { ui_.dumpScreen(); return; }
  if (!strcmp(command, "status")) {
    char temperature[20] = "null", humidity[20] = "null";
    if (hardware_.environment().valid) {
      snprintf(temperature, sizeof(temperature), "%.2f", hardware_.environment().temperatureC);
      snprintf(humidity, sizeof(humidity), "%.2f", hardware_.environment().humidityRh);
    }
    Serial.printf("{\"version\":\"0.7.0\",\"heap\":%u,\"battery\":%u,\"voltage\":%.3f,\"sensor\":%s,\"temperature\":%s,\"humidity\":%s,\"wifi\":%s,\"ip\":\"%s\",\"i2c_errors\":%lu,\"page\":%u,\"display_tiles\":%lu}\n",
      ESP.getFreeHeap(), hardware_.battery().percent, hardware_.battery().voltage, hardware_.environment().valid ? "true" : "false",
      temperature, humidity, connectivity_.wifiConnected() ? "true" : "false",
      WiFi.localIP().toString().c_str(), (unsigned long)hardware_.i2cErrors(), static_cast<unsigned>(page_), (unsigned long)ui_.lastTransferredTiles()); return;
  }
  if (!strcmp(command, "home")) page_ = Page::Home;
  else if (!strcmp(command, "clock")) page_ = Page::Clock;
  else if (!strcmp(command, "phrase-next")) NekoPhrases::next();
  else if (!strcmp(command, "cat")) page_ = Page::Cat;
  else if (!strcmp(command, "diag")) { Serial.println("[NEKO] Open diagnostics with KEY+BOOT held for 2 seconds."); return; }
  else if (!strcmp(command, "wifi")) { if (!diagnosticOpen_) { Serial.println("[NEKO] Use KEY+BOOT, then Connessioni."); return; } web_.openSetupWindow(600); connectivity_.openProvisioningWindow(300); page_ = Page::Connectivity; }
  else if (!strcmp(command, "pet")) { mood_ = CatMood::Happy; moodUntilMs_ = millis() + 7000; }
  else if (!strncmp(command, "timer ", 6)) {
    const uint32_t id = scheduler_.startTimer(strtoul(command + 6, nullptr, 10), "Timer USB");
    Serial.printf("[NEKO] timer=%lu\n", (unsigned long)id); page_ = Page::Timers;
  } else if (!strcmp(command, "pause") || !strcmp(command, "resume") || !strcmp(command, "cancel")) {
    for (const auto &timer : scheduler_.timers()) if (timer.active) {
      const uint32_t id = timer.id;
      if (!strcmp(command, "pause")) scheduler_.pauseTimer(id);
      else if (!strcmp(command, "resume")) scheduler_.resumeTimer(id);
      else scheduler_.cancelTimer(id);
      break;
    }
  } else { Serial.println("help | status | home | cat | pet | diag | timer <seconds> | pause | resume | cancel | wifi | screen"); return; }
  render(); Serial.println("[NEKO] OK");
}
void NekoApp::tick() {
  pollButtons(); serialTick(); connectivity_.tick(); web_.tick(); scheduler_.tick(time(nullptr));
  const int blePage = connectivity_.takePageRequest(), webPage = web_.takePageRequest();
  if (blePage >= 0 || webPage >= 0) { const auto requested=static_cast<Page>(blePage>=0?blePage:webPage); if(diagnosticOpen_||!NekoNavigation::isDiagnostic(requested)){page_=requested;render();} }
  if (web_.takePet()) { mood_ = CatMood::Happy; moodUntilMs_ = millis() + 7000; }
  if (connectivity_.takeChanged() || web_.takeChanged() || scheduler_.revision() != lastRevision_) {
    lastRevision_ = scheduler_.revision(); render();
  }
  if (appliedProfile_ != settings_.get().powerProfile) applyPowerProfile();
  updateMood();
  if (millis() - lastEnvironmentMs_ >= 60000) { lastEnvironmentMs_ = millis(); hardware_.readEnvironment(); render(); }
  if (millis() - lastBatteryMs_ >= 30000) {
    lastBatteryMs_ = millis(); hardware_.readBattery();
    if (hardware_.battery().critical && !hardware_.battery().chargingEstimate) {
      render("Energia critica. Riposo per 30 minuti."); delay(1500); ui_.powerSave(true);
      esp_sleep_enable_ext1_wakeup_io(1ULL << Board::KEY, ESP_EXT1_WAKEUP_ANY_LOW);
      esp_sleep_enable_timer_wakeup(30ULL * 60ULL * 1000000ULL); esp_deep_sleep_start();
    }
  }
  uint32_t interval = page_ == Page::Timers || page_ == Page::Clock ? 1000 : 5000;
  if (page_ == Page::Home || page_ == Page::Cat)
    interval = appliedProfile_ == PowerProfile::Night ? 600 : appliedProfile_ == PowerProfile::Saver ? 300 : 150;
  if (millis() - lastRenderMs_ >= interval) render();
  if (millis() > 60000) crashBootCount = 0;
  delay(3);
}
