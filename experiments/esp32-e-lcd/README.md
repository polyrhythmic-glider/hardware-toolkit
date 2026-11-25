# ESP32-S3 RLCD workspace

Workspace per Waveshare ESP32-S3-RLCD-4.2: dai test hardware minimi a un firmware
completo da assistente da scrivania.

- [`rlcd_explorer/`](rlcd_explorer/): test minimo ufficiale U8g2/ST7305, pronto per il primo caricamento.
- [`NekoAssistant/`](NekoAssistant/): sketch principale, verificabile e caricabile
  direttamente con Arduino IDE 2.x (ESP32 core 3.3.x), senza PlatformIO.
- [`neko-assistant/`](neko-assistant/): firmware ESP-IDF commerciale/modulare “Neko”,
  companion Web Bluetooth, design spec, protocollo BLE, test plan e checklist di riferimento.
- [Repository ufficiale Waveshare](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2): schemi ed esempi per sensore, RTC, SD, audio, LVGL, ESP-IDF ed ESPHome.
