# Test ufficiale RLCD

Questa cartella contiene il test minimo `10_U8G2_Test` ufficiale per **Waveshare ESP32-S3-RLCD-4.2**. Sul display deve apparire un contatore grande, insieme a FPS e tempo di aggiornamento.

## Requisiti

- Arduino IDE 2.x
- pacchetto schede `esp32` di Espressif **3.3.0 o successivo**
- libreria `U8g2` installata dal Library Manager

## Impostazioni Arduino IDE

- Board: `ESP32S3 Dev Module`
- USB CDC On Boot: `Enabled`
- CPU Frequency: `240MHz (WiFi)`
- Flash Mode: `QIO 80MHz`
- Flash Size: `16MB (128Mb)`
- Partition Scheme: `16M Flash (3MB APP/9.9MB FATFS)`
- PSRAM: `QSPI PSRAM`
- Upload Mode: `UART0 / Hardware CDC`
- USB Mode: `Hardware CDC and JTAG`

Apri `rlcd_explorer.ino`, collega la USB-C, scegli `COM8` e premi Upload. Se l'upload non parte, tieni premuto `BOOT`, premi brevemente `PWR` e poi rilascia `BOOT` quando Arduino IDE mostra `Connecting...`.

Il pannello non ha retroilluminazione: per valutarlo mettilo sotto una lampada o vicino a una finestra.

## Pin usati

| Funzione | GPIO |
|---|---:|
| RLCD SCK | 11 |
| RLCD MOSI | 12 |
| RLCD DC | 5 |
| RLCD CS | 40 |
| RLCD RST | 41 |
| BOOT | 0 |

Lo sketch e il driver `ST7305_U8g2.*` provengono dall'esempio ufficiale `10_U8G2_Test`, licenza Apache-2.0. La copia completa del repository Waveshare è nella cartella `waveshare-official`.
