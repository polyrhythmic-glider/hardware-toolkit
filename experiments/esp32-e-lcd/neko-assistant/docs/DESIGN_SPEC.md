# Neko — verifica hardware, specifica e UX

## 1. Verifica hardware

Profilo supportato: `WS_RLCD42_R1`, corrispondente allo schema ufficiale pubblicato al 15 luglio 2026. Waveshare non pubblica al momento una matrice di revisioni per questo prodotto; una scheda futura con pin diversi richiederà un nuovo profilo, non un rilevamento euristico.

Fonti primarie: [pagina ufficiale](https://docs.waveshare.com/ESP32-S3-RLCD-4.2), [schema](https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf), [repository Waveshare](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2), [risorse e datasheet](https://docs.waveshare.com/ESP32-S3-RLCD-4.2/Resources-And-Documents).

| Periferica | Modello | Bus / indirizzo | GPIO verificati | Uso Neko | Fonte e note |
|---|---|---|---|---|---|
| MCU | ESP32-S3-WROOM-1-N16R8 | — | — | 16 MB Flash, 8 MB PSRAM | schema + pagina prodotto |
| RLCD 4,2″ | ST7305, 300×400 fisici | SPI3, 24 MHz | SCLK 11, MOSI 12, DC 5, CS 40, RESET 41, TE 6 | UI logica 400×300, rotazione `U8G2_R1`; TE non necessario al driver | esempi 11_U8G2 e XiaoZhi ufficiali |
| RTC | PCF85063ATL | I²C 0x51 | SDA 13, SCL 14 | bootstrap ora, verifica OS flag, aggiornamento SNTP | schema + 04_I2C_PCF85063 |
| T/RH | SHTC3 | I²C 0x70 | SDA 13, SCL 14 | ambiente interno, CRC su entrambe le parole | schema + 05_I2C_SHTC3 |
| Codec uscita | ES8311, 0x18 | I²C + I²S | MCLK 16, BCLK 9, LRCLK 45, DOUT 8 | toni, sveglia, speaker test | 07_Audio_Test e `board_cfg.txt` |
| ADC microfoni | ES7210, 0x40 | I²C + I²S TDM | MCLK 16, BCLK 9, LRCLK 45, DIN 10 | due microfoni, livello e registrazione diagnostica | schema + 07_Audio_Test |
| Amplificatore | NS4150B | GPIO enable | PA_CTRL 46 | abilita speaker | schema + esempio audio |
| microSD | slot TF FAT32 | SDMMC 1 bit | CLK 38, CMD 21, D0 39 | log, registrazioni, OTA locale | 06_SD_Card |
| Batteria | 18650 + ETA6098 + S8261 | ADC1 CH3 | GPIO4 | tensione calibrata, curva e shutdown | schema: partitore 200 kΩ / 100 kΩ, rapporto 3 |
| KEY | pulsante attivo basso | GPIO | GPIO18, pull-up | short/double/long/very-long | schema + factory/audio example |
| BOOT | pulsante attivo basso | GPIO0 | GPIO0 | download; non usato nella UX normale | schema + pagina prodotto |
| PWR | circuito hardware | non esposto come GPIO | nessuno | click accende, pressione lunga spegne | pagina prodotto; il firmware non lo intercetta |
| USB-C | USB D+/D− | USB native | GPIO19/20 | flash e seriale | schema |
| Wi‑Fi/BLE | radio ESP32-S3 | 2,4 GHz / BLE 5 LE | interni | SNTP, OTA, companion | pagina prodotto |

Il pin TE è GPIO6 negli esempi ufficiali, non GPIO17. Il segnale STAT del caricatore e VBUS non sono instradati a GPIO: `charging_estimate` è quindi dichiaratamente una stima dalla pendenza filtrata della tensione. Il LED CHG resta un indicatore hardware.

## 2. Analisi esempi Waveshare

| Esempio ufficiale | Fa | Riutilizzo | Limite corretto in Neko |
|---|---|---|---|
| 01_WIFI_AP | AP dimostrativo | riferimento per modalità radio | Neko preferisce provisioning BLE cifrato e non avvia un captive portal permanente |
| 02_WIFI_STA | connessione STA | sequenza driver/eventi | credenziali tolte dal sorgente, retry/timeout e modem sleep aggiunti |
| 03_ADC_Test | ADC1 CH3 ×3 | pin, attenuazione, calibrazione | sostituita percentuale lineare con curva Li-ion, media, arrotondamento e isteresi |
| 04_I2C_PCF85063 | lettura RTC con SensorLib | indirizzo e mappa registri | niente set fisso al boot; OS flag, timezone e SNTP bidirezionale |
| 05_I2C_SHTC3 | lettura T/RH | comandi, indirizzo, CRC | sleep fra letture, validazione e offset configurabile |
| 06_SD_Card | mount SDMMC 1 bit | CLK/CMD/D0 e FATFS | assenza/rimozione sono recuperabili, nessuna formattazione automatica |
| 07_Audio_Test | loop mic→speaker | `codec_board` ufficiale, pin e codec | audio in task con coda; niente loop bloccante continuo |
| 08_LVGL_V8 | UI colore | orientamento e port display | non adottato: memoria/refresh e asset colore non adatti al prodotto |
| 09_LVGL_V9 | UI colore | verifica API moderna | Neko usa U8g2 1 bpp più piccolo e specifico per RLCD |
| 10_FactoryProgram | test integrato | sequenza self-test e pulsanti | separato in diagnostica non distruttiva |
| 11_U8G2_Test | ST7305 1 bpp | driver ufficiale e refresh a tile | layout reale, aggiornamenti regionali e frequenza ridotta |
| XiaoZhi V2.1 | voce e rete | conferma audio/pin e interfacce AI future | il client cloud e i modelli vocali non sono dichiarati parte di Neko 0.1 |

Waveshare richiede ESP-IDF 5.5.0 o superiore e mostra 5.5.1/5.5.2; il progetto vincola `>=5.5,<6.0`.

## 3. Specifica funzionale

Al boot Neko inizializza NVS, controlla rollback/boot loop, legge RTC, rende subito una home offline e avvia periferiche indipendenti. Un guasto SHTC3, SD o audio non blocca l’orologio. Quando esistono credenziali, il Wi‑Fi resta acceso solo nella finestra del profilo; SNTP aggiorna il sistema e riscrive il PCF85063. BLE resta il canale locale per configurazione e controllo.

Timer (quattro), sveglie ricorrenti (otto) e promemoria (otto) sono persistenti. Un timer in esecuzione durante un reset torna in pausa per evitare una scadenza falsa; sveglie e promemoria usano tempo assoluto e recuperano offline. Prima del deep sleep il prossimo evento arma il timer RTC interno ESP32; `RtcManager::configure_alarm` rende disponibile anche l’allarme PCF85063, ma non viene usato come wake GPIO finché Waveshare non documenta ufficialmente la connessione di RTC_INT al SoC.

## 4. User experience

Flusso di avvio: splash/home immediata → validazione RTC → periferiche → BLE advertising → finestra Wi‑Fi, SNTP e ritorno offline. Se l’ora non è valida compare `ORA: NON AFFIDABILE`; non si fabbrica una data.

Configurazione: associare via BLE → pressione lunga KEY → inviare SSID/password entro due minuti → connessione e SNTP. La companion non conserva la password. KEY breve cambia pagina; doppio da Home avvia un timer da cinque minuti, altrove torna Home; lungo apre Impostazioni/provisioning; molto lungo apre Diagnostica. PWR conserva sempre il comportamento elettrico Waveshare.

In carica il LED CHG hardware rimane la fonte primaria; la UI usa solo `STIMA DA TREND`. Offline restano RTC, timer, sveglie, promemoria, sensore, display e KEY. In batteria critica si salva lo stato già transazionale, si spengono radio/audio, si mostra la schermata finale e si entra in deep sleep risvegliabile da KEY.

## 5. Wireframe

Home scelta, 400×300:

```text
┌ NEKO ───────────────────────── WiFi  BLE  [BAT] 85 ┐
│                                                     │
│   09:42                         /\_/\               │
│   MAR 15 LUG 2026              ( • • )  cravatta    │
│   ORA: SNTP + RTC               \ ^ /               │
│                                  |▼|                │
│ ┌────────────────────────┐                           │
│ │ INTERNO 24.1 C  48 %RH │                           │
│ │ PROSSIMO MER 07:30     │                           │
│ └────────────────────────┘                           │
│ ┌─────────────────────────────────────────────────┐ │
│ │ Tutto sotto controllo.                          │ │
│ │ KEY pagina · doppio rapido · lungo menu          │ │
│ └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

Pagine dati (Ambiente, Batteria, Connessioni, Audio, Sistema, Diagnostica):

```text
┌ TITOLO ──────────────────────── WiFi BLE [BAT] ┐
│ ETICHETTA             VALORE                    │
│ ─────────────────────────────────────────────── │
│ ETICHETTA             VALORE                    │
│ ─────────────────────────────────────────────── │
│ Nota tecnica o stato recuperabile               │
│                                                 │
│ KEY: pagina successiva      lungo: menu         │
└─────────────────────────────────────────────────┘
```

Orologio/Notte usano ora grande; Timer/Sveglie/Promemoria mostrano il prossimo evento e il canale di controllo; Impostazioni mostra nome/timezone/profilo; Cat fullscreen usa avatar 162×210 procedurale; Aggiornamento mostra canale, verifica e rollback. Le quindici pagine sono enumerate in `ui_manager.hpp` e rese in `ui_manager.cpp`.

## 6. Design system

Griglia 8 px, margine 10–20 px, linee 1 px, radius 5 px. Font U8g2 5×8 per hint, 6×13 per corpo, 9×18 bold per valori, Logisoso 50 per ora. Niente gradienti o emoji. Retini sono riservati a stato disabilitato; icone Wi‑Fi/BLE/batteria sono primitive. Modali occupano il pannello inferiore; toast coincidono con il messaggio del gatto. L’avatar occupa circa 54×70 unità scalabili e non richiede Flash bitmap; animazione occhi/coda avviene modificando poche regioni.
