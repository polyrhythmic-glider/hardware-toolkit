# Neko Assistant per Arduino IDE

Questa è la build da usare e caricare direttamente con **Arduino IDE 2.x**.
Non richiede PlatformIO.

## Installazione una tantum

1. In Board Manager installare **esp32 by Espressif Systems 3.3.10** (o 3.3.x).
2. Dal Library Manager installare **U8g2 2.36.19 o successiva**. Il driver/adattatore
   ST7305 specifico della scheda è già incluso nello sketch dalla copia ufficiale
   Waveshare presente nel repository.
3. Aprire `NekoAssistant.ino`.

## Impostazioni Arduino IDE

Selezionare **ESP32S3 Dev Module** e configurare:

| Opzione | Valore |
|---|---|
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240 MHz |
| Flash Mode | QIO 80 MHz |
| Flash Size | 16 MB |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0 / Hardware CDC |

Scegliere la porta COM della scheda, quindi usare **Verify** e **Upload**.

Attenzione: **Flash QIO** e **PSRAM OPI** sono due impostazioni diverse. Se il
Monitor Seriale mostra `quad_psram: PSRAM chip is not connected`, in Tools è
stata selezionata per errore `QSPI PSRAM`. Selezionare `OPI PSRAM`, ricompilare e
caricare nuovamente. Lo sketch può funzionare anche con PSRAM `Disabled`, ma OPI
è l'impostazione corretta per il modulo N16R8.

L'esito dell'ultima compilazione reale, incluse versioni, occupazione e hash dei
binari, è registrato in [`BUILD_VERIFIED.md`](BUILD_VERIFIED.md).

## Funzioni e comandi

- il pulsante **KEY è GPIO18, attivo LOW**: quello integrato sulla scheda è già
  collegato; per un tasto esterno collegare un contatto momentaneo fra GPIO18 e
  GND. Lo sketch abilita `INPUT_PULLUP`, quindi non serve una resistenza esterna;
- anche **BOOT/GPIO0** cambia pagina dopo che il firmware è partito. Non tenerlo
  premuto durante reset o accensione, perché in quel momento seleziona il bootloader;
- il firmware incorpora senza modifiche il driver `multi_button` dell'esempio
  ufficiale Waveshare `07_Audio_Test`: timer ESP ogni 5 ms, debounce a tre
  campioni e callback single/double/long. Come nello stesso esempio, due task
  FreeRTOS attendono direttamente gli EventGroup di GPIO18 e GPIO0; vengono
  avviati prima di SD, OTA e BLE. Nel Monitor Seriale a 115200 baud ogni
  gesto produce una riga `[KEY/Waveshare] ...`; in Diagnostica i
  livelli `KEY18` e `BOOT0` devono passare da `1` a `0` durante la pressione;
- click KEY: pagina successiva;
- doppio click: timer rapido da 5 minuti;
- pressione lunga: pagina connettività e finestra provisioning BLE di 2 minuti;
- pressione di 5 secondi: diagnostica;
- RTC PCF85063, SHTC3, batteria, microSD, Wi-Fi/SNTP e BLE sono inizializzati
  usando il pinout ufficiale della scheda;
- una microSD FAT32 con `neko-update.bin` e il marker vuoto
  `neko-update.ready` avvia un aggiornamento solo sopra il 50% di batteria o con
  una tendenza di carica positiva.

Il gatto usa quattro vere sprite XBM 1-bit (idle, felice, assonnato e allerta),
senza antialiasing. Le bitmap compilate sono in `NekoCatSprites.h`; il foglio di
anteprima è `cat-sprites-preview.png` e lo script riproducibile è in
`tools/generate_cat_sprites.py`.

Tutto il testo UI usa `drawUTF8()` e font U8g2 completi (`*_tf`), quindi lettere
italiane come `à`, `è`, `é`, `ì`, `ò` e `ù` sono decodificate correttamente.

Il companion Web Bluetooth resta in `../neko-assistant/companion` e usa lo stesso
protocollo BLE. Audio avanzato ES8311/ES7210 resta intenzionalmente diagnostico
nella build Arduino: il percorso codec ufficiale richiede i componenti ESP-IDF
vendorizzati nell'esempio Waveshare; la build ESP-IDF completa resta in
`../neko-assistant` come riferimento, ma non serve per caricare questo sketch.
