#include "NekoApp.h"

#include <SD_MMC.h>
#include <Update.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <sys/time.h>
#include "button_bsp.h"

RTC_DATA_ATTR static uint32_t crashBootCount = 0;

NekoApp::NekoApp() : connectivity_(settings_, hardware_, scheduler_) {}

void NekoApp::begin() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\nNeko Assistant Arduino 0.2.0");
  // Configurazione ufficiale Waveshare: GPIO0 e GPIO18 input pull-up,
  // esp_timer a 5 ms e callback multi_button.
  Custom_ButtonInit();
  displayMutex_ = xSemaphoreCreateMutex();
  pinMode(Board::PA_ENABLE, OUTPUT);
  digitalWrite(Board::PA_ENABLE, LOW);

  const esp_reset_reason_t reason = esp_reset_reason();
  const bool crashed = reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
                       reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT;
  crashBootCount = crashed ? crashBootCount + 1 : 0;
  settings_.begin();
  setenv("TZ", settings_.get().timezone, 1);
  tzset();
  hardware_.begin(settings_.get().sensorOffsetC);
  scheduler_.begin();
  bootstrapTime();
  ui_.begin();
  render(crashBootCount >= 3 ? "SAFE MODE: radio disattivata" : "Neko è pronto.");
  // Come nell'esempio Waveshare, i task consumano direttamente gli EventGroup.
  // Partono prima di SD, OTA e BLE: il tasto resta operativo anche se una di
  // queste inizializzazioni richiede molto tempo.
  xTaskCreatePinnedToCore(keyTaskEntry, "KEY_LoopTask", 4096, this, 4,
                          &keyTaskHandle_, 1);
  xTaskCreatePinnedToCore(bootTaskEntry, "BOOT_LoopTask", 4096, this, 3,
                          &bootTaskHandle_, 1);
  Serial.printf("[KEY/Waveshare] task avviati GPIO18=%d GPIO0=%d\n",
                gpio_get_level(static_cast<gpio_num_t>(Board::KEY)),
                gpio_get_level(static_cast<gpio_num_t>(Board::BOOT_KEY)));
  hardware_.mountSd();
  checkSdUpdate();
  if (crashBootCount < 3) connectivity_.begin();
  applyPowerProfile();
  render();
}

void NekoApp::bootstrapTime() {
  tm rtc{};
  if (!hardware_.readRtc(rtc)) return;
  const time_t epoch = mktime(&rtc);
  if (epoch < 1640995200) return;
  timeval tv{epoch, 0};
  settimeofday(&tv, nullptr);
}

void NekoApp::applyPowerProfile() {
  switch (settings_.get().powerProfile) {
    case PowerProfile::Performance: setCpuFrequencyMhz(240); break;
    case PowerProfile::Balanced: setCpuFrequencyMhz(160); break;
    case PowerProfile::Saver: setCpuFrequencyMhz(80); break;
    case PowerProfile::Night: setCpuFrequencyMhz(80); break;
    case PowerProfile::Critical: setCpuFrequencyMhz(40); break;
  }
}

void NekoApp::checkSdUpdate() {
  if (!hardware_.sdOk() || !SD_MMC.exists("/neko-update.ready") || !SD_MMC.exists("/neko-update.bin")) return;
  if (hardware_.battery().percent < 50 && !hardware_.battery().chargingEstimate) {
    scheduler_.pushNotification("OTA SD bloccato: batteria sotto 50%", 2, 60);
    return;
  }
  File firmware = SD_MMC.open("/neko-update.bin", FILE_READ);
  if (!firmware || !Update.begin(firmware.size(), U_FLASH)) return;
  mood_ = CatMood::Updating;
  render("Aggiornamento da microSD...");
  const size_t written = Update.writeStream(firmware);
  const bool ok = written == firmware.size() && Update.end(true);
  firmware.close();
  if (ok) {
    SD_MMC.remove("/neko-update.ready");
    ESP.restart();
  }
  mood_ = CatMood::Error;
  strlcpy(message_, "Aggiornamento fallito", sizeof(message_));
}

void NekoApp::keyTaskEntry(void *argument) {
  static_cast<NekoApp *>(argument)->keyTask();
}

void NekoApp::bootTaskEntry(void *argument) {
  static_cast<NekoApp *>(argument)->bootTask();
}

void NekoApp::dispatchButtonEvent(EventBits_t received, const char *source) {
  Serial.printf("[KEY/Waveshare] %s bits=0x%02lx GPIO18=%d GPIO0=%d\n", source,
                static_cast<unsigned long>(received),
                gpio_get_level(static_cast<gpio_num_t>(Board::KEY)),
                gpio_get_level(static_cast<gpio_num_t>(Board::BOOT_KEY)));
  if (received & 0x01) onShortPress();
  else if (received & 0x02) onDoublePress();
  else if (received & 0x04) onLongPress();
}

void NekoApp::keyTask() {
  constexpr EventBits_t events = 0x01 | 0x02 | 0x04;
  for (;;) {
    const EventBits_t received = xEventGroupWaitBits(GP18ButtonGroups, events,
                                                      pdTRUE, pdFALSE, portMAX_DELAY);
    dispatchButtonEvent(received, "GPIO18");
  }
}

void NekoApp::bootTask() {
  constexpr EventBits_t events = 0x01 | 0x02 | 0x04;
  for (;;) {
    const EventBits_t received = xEventGroupWaitBits(BootButtonGroups, events,
                                                      pdTRUE, pdFALSE, portMAX_DELAY);
    dispatchButtonEvent(received, "GPIO0");
  }
}

void NekoApp::onShortPress() {
  Serial.println("[KEY] click");
  page_ = static_cast<Page>((static_cast<uint8_t>(page_) + 1) % static_cast<uint8_t>(Page::Count));
  render();
}

void NekoApp::onDoublePress() {
  Serial.println("[KEY] doppio click");
  scheduler_.startTimer(300, "Pausa breve");
  page_ = Page::Home;
  mood_ = CatMood::Timer;
  moodUntilMs_ = millis() + 4000;
  render("Timer da 5 minuti avviato");
}

void NekoApp::onLongPress() {
  Serial.println("[KEY] pressione lunga");
  connectivity_.openProvisioningWindow(120);
  page_ = Page::Connectivity;
  mood_ = CatMood::Alert;
  moodUntilMs_ = millis() + 8000;
  render("Provisioning BLE: 2 minuti");
}

void NekoApp::onVeryLongPress() {
  Serial.println("[KEY] pressione 5 secondi");
  page_ = Page::Diagnostics;
  render("Autodiagnostica");
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
  if (hardware_.battery().critical) {
    mood_ = CatMood::LowBattery; strlcpy(message_, "Batteria critica", sizeof(message_));
  } else if (hardware_.environment().valid && hardware_.environment().temperatureC >= 29) {
    mood_ = CatMood::Hot; strlcpy(message_, "Qui fa piuttosto caldo.", sizeof(message_)); moodUntilMs_ = millis() + 15000;
  } else if (hardware_.environment().valid && hardware_.environment().humidityRh >= 70) {
    mood_ = CatMood::Humid; strlcpy(message_, "Aria molto umida.", sizeof(message_)); moodUntilMs_ = millis() + 15000;
  } else if (moodUntilMs_ && static_cast<int32_t>(millis() - moodUntilMs_) >= 0) {
    mood_ = connectivity_.wifiConnected() || !settings_.get().ssid[0] ? CatMood::Idle : CatMood::Offline;
    strlcpy(message_, mood_ == CatMood::Offline ? "Wi-Fi non raggiungibile" : "Tutto tranquillo.", sizeof(message_));
    moodUntilMs_ = 0;
  }
}

void NekoApp::render(const char *message) {
  if (message) strlcpy(message_, message, sizeof(message_));
  if (displayMutex_) xSemaphoreTake(displayMutex_, portMAX_DELAY);
  ui_.render(page_, mood_, message_, settings_.get(), hardware_, scheduler_,
             connectivity_.wifiConnected(), connectivity_.rssi(), connectivity_.bleConnected(),
             connectivity_.timeValid());
  if (displayMutex_) xSemaphoreGive(displayMutex_);
  lastRenderMs_ = millis();
}

void NekoApp::tick() {
  connectivity_.tick();
  scheduler_.tick(time(nullptr));
  updateMood();
  if (millis() - lastEnvironmentMs_ >= 60000) {
    lastEnvironmentMs_ = millis(); hardware_.readEnvironment(); updateMood();
  }
  if (millis() - lastBatteryMs_ >= 30000) {
    lastBatteryMs_ = millis(); hardware_.readBattery(); updateMood();
    if (hardware_.battery().critical && !hardware_.battery().chargingEstimate) {
      render("Batteria critica: sleep 30 min"); delay(1500); ui_.powerSave(true);
      esp_sleep_enable_ext1_wakeup_io(1ULL << Board::KEY, ESP_EXT1_WAKEUP_ANY_LOW);
      esp_sleep_enable_timer_wakeup(30ULL * 60ULL * 1000000ULL);
      esp_deep_sleep_start();
    }
  }
  if (millis() - lastRenderMs_ >= (page_ == Page::Clock ? 60000UL : 180000UL)) render();
  delay(settings_.get().powerProfile == PowerProfile::Performance ? 2 : 10);
}
