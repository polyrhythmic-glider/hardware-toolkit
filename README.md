# Hardware Toolkit

Esperimenti hardware/software per ESP32, encoder rotativi, OLED I2C e multiplexer PCA9548A.

## Progetti

- `dueENcoder/`: sketch Arduino principale per ESP32 DevKit V1 / ESP32-WROOM-32, 4 OLED SSD1306 e 4 encoder.
- `oledMultiplexerTest/`: sketch minimale di test per verificare PCA9548A e i 4 OLED senza la UI completa.

La parte PlatformIO precedente e' stata rimossa: il repository ora e' centrato sugli sketch Arduino.

## Hardware Principale

- ESP32 DevKit V1 / ESP32-WROOM-32
- PCA9548A I2C multiplexer, indirizzo `0x70`
- 4 OLED SSD1306 0.96", 128x64, I2C, indirizzo `0x3C`
- 4 encoder rotativi con pulsante
- Resistenze di pull-up I2C da `4.7k` consigliate

## Collegamenti

### ESP32 -> PCA9548A

| PCA9548A | ESP32 |
| --- | --- |
| VCC / VIN | 3V3 |
| GND | GND |
| SDA | GPIO21 / D21 |
| SCL | GPIO22 / D22 |

### OLED -> PCA9548A

| OLED | PCA9548A |
| --- | --- |
| OLED 1 SDA/SCL | SD0 / SC0 |
| OLED 2 SDA/SCL | SD1 / SC1 |
| OLED 3 SDA/SCL | SD2 / SC2 |
| OLED 4 SDA/SCL | SD3 / SC3 |

Tutti gli OLED vanno alimentati a `3V3` e `GND` comuni.

### Encoder -> ESP32

| Encoder | CLK / A | DT / B | SW |
| --- | --- | --- | --- |
| Encoder 1 | GPIO13 / D13 | GPIO23 / D23 | GPIO14 / D14 |
| Encoder 2 | GPIO27 / D27 | GPIO26 / D26 | GPIO25 / D25 |
| Encoder 3 | GPIO33 / D33 | GPIO32 / D32 | GPIO18 / D18 |
| Encoder 4 | GPIO19 / D19 | GPIO16 / RX2 | GPIO17 / TX2 |

Ogni encoder usa `3V3` e `GND`. Nella UI fisica, schermo 1 sta sopra encoder 1, schermo 2 sopra encoder 2, e cosi' via.

## Pull-Up I2C

Nota di merito enorme: le resistenze di pull-up sono state la svolta.

Configurazione consigliata:

```text
3V3 ---[4.7k]--- SDA principale ESP32/PCA
3V3 ---[4.7k]--- SCL principale ESP32/PCA
```

Se il bus e' instabile, aggiungere pull-up anche sui canali downstream del PCA:

```text
3V3 ---[4.7k]--- SD0
3V3 ---[4.7k]--- SC0
...
3V3 ---[4.7k]--- SD3
3V3 ---[4.7k]--- SC3
```

Senza pull-up adeguate, gli OLED possono essere visibili nello scan I2C ma mostrare caratteri corrotti, righe verticali, schermate bianche o non inizializzarsi in modo affidabile.

## Firmware `dueENcoder`

Funzioni principali:

- UI multi-schermo con pannelli utility.
- Menu comandato da Encoder 4, quindi il menu principale vive sullo schermo 4.
- Encoder view con i primi 3 schermi in stile contagiri per Encoder 1, 2 e 3, e schermo 4 come summary dei click.
- Waveform view con parametri controllati da Encoder 1 e 2.
- Pong wide su 4 OLED, con campo logico largo `4 x 128` e paddle controllati da Encoder 1 e 4.
- Modalita' debug runtime: tenere premuti i pulsanti degli encoder 1 e 4 per circa 1 secondo.

Flag utili in `dueENcoder/AppConfig.h`:

```cpp
constexpr bool MULTI_SCREEN_UI = true;
constexpr bool WIDE_PONG = true;
constexpr bool DEBUG_SERIAL = false;
```

Importante: la rilevazione del PCA9548A non dipende piu' dal debug seriale. Il mux viene cercato sempre durante `setupDisplay()`.

## Librerie Arduino

- `Wire`
- `Adafruit_GFX`
- `Adafruit_SSD1306`

## Test Hardware

Usare `oledMultiplexerTest/oledMultiplexerTest.ino` quando serve isolare il problema hardware:

- verifica presenza del PCA9548A a `0x70`
- seleziona i canali `0..3`
- inizializza un OLED per canale
- disegna pattern diversi su ogni display

Se questo sketch funziona ma `dueENcoder` no, il problema e' nel firmware UI. Se questo sketch non funziona, controllare cablaggio, alimentazione, GND comune e pull-up.
