# Revisione requisiti

## Implementato in 0.1

- Pin e periferiche verificati: `board_config.hpp`, `DESIGN_SPEC.md`.
- ST7305/RLCD monocromatico, home e 15 pagine: `display_manager.cpp`, `ui_manager.cpp`.
- Quindici espressioni e stati prioritari/cooldown: `cat_avatar.cpp`, `cat_state_machine.cpp`.
- RTC offline + OS flag + POSIX Europe/Rome + SNTP→RTC: `rtc_manager.cpp`, `time_service.cpp`.
- SHTC3 con doppio CRC, sleep, range e offset: `sensor_manager.cpp`.
- Batteria calibrata, rapporto 3, curva, filtro, 5%, isteresi e shutdown: `battery_manager.cpp`, `power_manager.cpp`.
- KEY short/double/long/very-long; PWR non alterato: `input_manager.cpp`, `app_controller.cpp`.
- Timer multipli, pause/resume; sveglie e promemoria persistenti: `timer_manager.cpp`, `alarm_manager.cpp`.
- BLE standard/custom, pairing/bonding, input validation e provisioning fisico: `ble_manager.cpp`, `BLE_PROTOCOL.md`.
- Wi‑Fi retry/timeout/modem sleep/finestra offline: `wifi_manager.cpp`, `app_controller.cpp`.
- ES8311/ES7210, toni, volume/mute e cattura PCM diagnostica: `audio_manager.cpp`.
- FAT32 opzionale, stato, log e fallback: `storage_manager.cpp`.
- PM, cinque profili, light/deep sleep e wake KEY/timer: `power_manager.cpp`.
- NVS schema/CRC/default/reset e scheduler: `settings_manager.cpp`.
- HTTPS/SD OTA, CA, dual-slot, verifica, rollback, soglia batteria: `ota_manager.cpp`, `partitions.csv`.
- Event bus, task separati, ownership e safe mode: `event_bus.cpp`, `ARCHITECTURE.md`, `app_controller.cpp`.
- Companion Web Bluetooth: `companion/index.html`, `companion/app.js`.
- Test algoritmi e piano hardware/fault/consumi: `tests/host`, `TEST_PLAN.md`.

## Estensioni future dichiarate

- Wake word, VAD, AEC/NR e AI remota: interfaccia audio pronta, nessun modello vocale è incluso.
- API locale/captive portal: non necessari nel flusso BLE scelto; da aggiungere solo con threat model.
- Backup/esportazione completa configurazione e asset SD: storage è pronto, il formato backup non è ancora stabilizzato.
- Firma OTA/Secure Boot/Flash+NVS encryption richiedono chiavi di produzione dell’integratore.
- Il segnale caricatore non è leggibile dal SoC: la UI espone solo una stima e il LED CHG hardware.
- Wake via pin PCF85063 RTC_INT sospeso finché Waveshare non ne documenta il GPIO; attivo il wake timer RTC interno ESP32.
- Il self-test 0.1 è una pagina diagnostica e una checklist guidata; la procedura interattiva passo-passo sarà un’estensione.
