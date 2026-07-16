# Neko Assistant

> La build principale richiesta per Arduino IDE è in
> [`../NekoAssistant`](../NekoAssistant). Questa cartella conserva la variante
> ESP-IDF e la documentazione architetturale di riferimento.

Firmware ESP-IDF modulare per **Waveshare ESP32-S3-RLCD-4.2**, progettato come
assistente da scrivania monocromatico, event-driven e power-aware. Il gatto è
disegnato proceduralmente sul display ST7305 e comunica stati, timer, allarmi,
ambiente, connettività e diagnostica senza dipendere dal cloud.

## Stato del progetto

- target verificato: ESP32-S3 N16R8, LCD riflettivo ST7305 400x300 in landscape;
- RTC PCF85063, SHTC3, ADC batteria, KEY, microSD, ES8311/ES7210 e audio I2S;
- UI U8g2 con refresh regionale, state machine del gatto e 15 pagine;
- timer, sveglie ricorrenti, promemoria, notifiche con cooldown e persistenza NVS;
- Wi-Fi STA/SNTP, BLE NimBLE cifrato, provisioning fisicamente autorizzato;
- OTA HTTPS e da microSD con rollback e controllo batteria;
- cinque profili energetici, light/deep sleep, safe mode e diagnostica;
- companion Web Bluetooth statico in `companion/`.

Le funzioni che richiedono hardware non esposto dalla scheda o una fase prodotto
successiva sono marcate con prove e motivazione in
[`docs/REQUIREMENTS_CHECKLIST.md`](docs/REQUIREMENTS_CHECKLIST.md).

## Prerequisiti

1. ESP-IDF **5.5.x o 6.x** installato ed esportato nella shell.
2. Repository padre completo: il progetto riusa i componenti ufficiali Waveshare
   presenti in `../waveshare-official`.
3. Cavo USB-C dati e, per audio/SD, una microSD FAT32 opzionale.

## Build e flash

Da una ESP-IDF PowerShell:

```powershell
cd neko-assistant
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

Oppure:

```powershell
.\tools\build.ps1 -Port COM5
```

Il primo avvio usa configurazione sicura: Wi-Fi non configurato, nome `Neko`,
timezone Europe/Rome, profilo Balanced. Una pressione lunga apre per due minuti
la finestra BLE che autorizza l'invio delle credenziali Wi-Fi.

## Comandi KEY

| Gesto | Azione |
|---|---|
| click | pagina successiva / conferma contestuale |
| doppio click | timer rapido 5 minuti e ritorno Home |
| pressione lunga (>=700 ms) | Settings + provisioning BLE per 2 minuti |
| pressione molto lunga (>=5 s) | Diagnostics |

## Companion Web Bluetooth

Servire `companion/` da `localhost` o HTTPS, quindi usare Chrome/Edge con Web
Bluetooth. Per esempio:

```powershell
cd companion
python -m http.server 8080
```

Aprire `http://localhost:8080`. La pagina legge batteria, temperatura, umidità e
ora e consente nome, notifica, sveglia e credenziali Wi-Fi. Il protocollo binario
versionato è in [`docs/BLE_PROTOCOL.md`](docs/BLE_PROTOCOL.md).

## Test

I test host verificano CRC SHTC3, conversioni BCD e curva batteria:

```powershell
.\tools\host-tests.ps1
```

Le prove su hardware, inclusi consumi, audio loopback, SD, sleep/wake, BLE e OTA,
sono definite in [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md).

## Documentazione

- [`docs/DESIGN_SPEC.md`](docs/DESIGN_SPEC.md): hardware verificato, requisiti e UX;
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): moduli, task, ownership e persistenza;
- [`docs/POWER_SAFETY.md`](docs/POWER_SAFETY.md): profili, batteria, sleep e limiti;
- [`docs/BLE_PROTOCOL.md`](docs/BLE_PROTOCOL.md): servizi, payload e sicurezza;
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md): strategia di test;
- [`docs/REQUIREMENTS_CHECKLIST.md`](docs/REQUIREMENTS_CHECKLIST.md): copertura del brief.

## Vincoli hardware dichiarati

- ETA6098 non espone STAT/VBUS all'ESP32: lo stato di carica firmware è una stima
  dalla tendenza di tensione; il LED CHG resta l'indicatore autorevole.
- Lo schema ufficiale non documenta un collegamento RTC_INT al SoC: il deep sleep
  usa il timer RTC interno ESP32 e KEY. Il driver PCF85063 prepara comunque i
  registri alarm per revisioni hardware che espongano INT.
- Il voice assistant cloud non è incluso: il percorso microfono e la diagnostica
  audio sono implementati, ma wake word/ASR/TTS richiedono un prodotto separato.

## Licenze e provenienza

Il codice Neko è progettato per questo repository. I componenti U8g2/ST7305 e
`codec_board` restano nelle rispettive cartelle ufficiali Waveshare e conservano
le loro licenze e avvisi originali.
