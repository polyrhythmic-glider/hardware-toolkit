# Hardware Toolkit

Repository di esperimenti hardware/software per ESP32, display OLED I2C, encoder rotativi, multiplexer PCA9548A e uscita audio I2S.

L'idea generale e' costruire un piccolo pannello fisico: quattro manopole con pulsante, quattro schermi OLED e un'uscita audio. Il firmware legge gli encoder, aggiorna i display, genera una forma d'onda e include anche una modalita' Pong distribuita sui quattro OLED.

## Stato del Repository

Questo repository e' stato ripulito e semplificato:

- il vecchio progetto `dueENcoder/` e' stato rinominato e riorganizzato come `synthProject/`;
- lo sketch separato `oledMultiplexerTest/` e' stato rimosso;
- la vecchia parte PlatformIO e' stata rimossa: il repository ora e' centrato su sketch Arduino;
- le funzioni di test hardware sono state integrate nel firmware principale tramite flag in `synthProject/AppConfig.h`.

## Progetto Principale

- `synthProject/`: sketch Arduino principale per ESP32 DevKit V1 / ESP32-WROOM-32, 4 OLED SSD1306, 4 encoder rotativi, multiplexer PCA9548A e DAC I2S PCM5102 / PCM5102A.

Per l'IDE Arduino e' importante che la cartella dello sketch e il file `.ino` abbiano lo stesso nome. Qui la coppia corretta e':

```text
synthProject/
└── synthProject.ino
```

## Cosa Fa

In parole semplici: l'ESP32 controlla quattro piccoli schermi e quattro manopole. Ogni manopola puo' cambiare valori o muovere elementi dell'interfaccia; gli schermi mostrano menu, strumenti di controllo, una waveform e un mini gioco.

In modo piu' tecnico:

- gli OLED condividono il bus I2C, ma vengono separati tramite un multiplexer PCA9548A;
- gli encoder sono letti tramite interrupt e tabella di quadratura;
- il menu principale e' comandato dall'Encoder 4;
- la modalita' waveform genera anche audio digitale I2S verso un DAC PCM5102;
- l'audio gira in un task FreeRTOS separato, cosi' i ridisegni OLED non interrompono il flusso I2S;
- la modalita' debug puo' stampare diagnostica seriale, scan I2C, valori encoder e heartbeat.

## Hardware

Componenti principali:

- ESP32 DevKit V1 / ESP32-WROOM-32
- PCA9548A I2C multiplexer, indirizzo tipico `0x70`
- 4 OLED SSD1306 0.96", 128x64, I2C, indirizzo tipico `0x3C`
- 4 encoder rotativi con pulsante
- DAC I2S PCM5102 / PCM5102A
- resistenze di pull-up I2C da `4.7k`, fortemente consigliate

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

Ogni encoder usa anche `3V3` e `GND`. Nel layout fisico: schermo 1 sopra encoder 1, schermo 2 sopra encoder 2, schermo 3 sopra encoder 3, schermo 4 sopra encoder 4.

### PCM5102 -> ESP32

| PCM5102 | ESP32 |
| --- | --- |
| VIN | 3V3 |
| GND | GND |
| BCK | GPIO5 |
| LCK / LRCK / WS | GPIO15 |
| DIN | GPIO4 |
| SCK | GND |

Il PCM5102A puo' lavorare in I2S a 3 fili usando il PLL interno agganciato a `BCK`, quindi non serve generare `SCK/MCLK` dall'ESP32. Tenere `SCK` a `GND` evita che il pin resti flottante.

Nota ESP32: `GPIO15` e' un pin di strap di boot. Con un ingresso DAC normale non dovrebbe disturbare, ma se la board non parte piu' dopo aver collegato il DAC, scollegare temporaneamente `LCK/LRCK` o spostarlo su un altro GPIO libero adatto a output.

## Pull-Up I2C

Le resistenze di pull-up sono state una parte importante della stabilizzazione hardware.

Configurazione consigliata sul bus principale:

```text
3V3 ---[4.7k]--- SDA principale ESP32/PCA
3V3 ---[4.7k]--- SCL principale ESP32/PCA
```

Se il bus resta instabile, si possono aggiungere pull-up anche sui canali downstream del PCA:

```text
3V3 ---[4.7k]--- SD0
3V3 ---[4.7k]--- SC0
...
3V3 ---[4.7k]--- SD3
3V3 ---[4.7k]--- SC3
```

Senza pull-up adeguate, gli OLED possono risultare visibili nello scan I2C ma mostrare caratteri corrotti, righe verticali, schermate bianche o inizializzarsi in modo non affidabile.

## Firmware `synthProject`

File principali:

- `synthProject.ino`: setup generale, loop principale, debug runtime e stato condiviso;
- `AppConfig.h`: pin, indirizzi I2C, flag di test, parametri UI, encoder, audio e Pong;
- `Hardware.cpp/.h`: inizializzazione OLED/PCA, scan I2C, gestione encoder e pulsanti;
- `Render.cpp/.h`: disegno delle schermate sui display;
- `Ui.cpp/.h`: logica dei menu e delle modalita';
- `Audio.cpp/.h`: generazione audio I2S e task FreeRTOS;
- `Pong.cpp/.h`: stato e fisica del mini gioco.

Modalita' principali:

- `ENCODER VIEW`: schermi 1-3 in stile contagiri per encoder 1-3; schermo 4 come summary dei click.
- `WAVEFORM VIEW`: editor grafico di waveform con uscita audio I2S sul PCM5102.
- `PONG`: mini Pong su campo largo `4 x 128` pixel, con paddle controllati da Encoder 1 e 4.

## Configurazione Tecnica

Flag utili in `synthProject/AppConfig.h`:

```cpp
constexpr bool DIRECT_OLED_TEST = false;
constexpr bool PCA_MUX_TEST = false;
constexpr bool MULTI_SCREEN_UI = true;
constexpr bool WIDE_PONG = true;
constexpr bool DEBUG_SERIAL = false;
```

Significato pratico:

- `DIRECT_OLED_TEST`: prova un OLED direttamente sul bus I2C principale, senza PCA9548A.
- `PCA_MUX_TEST`: usa il firmware principale per diagnosticare multiplexer e canali OLED.
- `MULTI_SCREEN_UI`: abilita la UI distribuita sui quattro schermi.
- `WIDE_PONG`: fa usare a Pong un campo largo quattro display invece di un solo OLED.
- `DEBUG_SERIAL`: abilita la seriale di debug gia' al boot.

Parametri rilevanti:

```cpp
constexpr uint8_t PCA9548A_ADDRESS = 0x70;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint32_t I2C_CLOCK_HZ = 400000;
constexpr int AUDIO_SAMPLE_RATE = 44100;
constexpr int AUDIO_BUFFER_SAMPLES = 256;
constexpr float AUDIO_BASE_FREQUENCY = 220.0f;
```

La rilevazione del PCA9548A non dipende dalla seriale di debug: il firmware cerca sempre il mux durante `setupDisplay()`.

## Debug e Diagnostica

Per attivare o disattivare il debug runtime, tenere premuti insieme i pulsanti degli encoder 1 e 4 per circa 1 secondo.

Quando il debug e' attivo:

- parte il Serial Monitor a `115200`;
- vengono stampati heartbeat, valori encoder e click;
- sono disponibili informazioni su bus I2C, PCA9548A e canali OLED;
- alcuni pannelli utility mostrano diagnostica a schermo.

Checklist rapida se qualcosa non funziona:

1. Se nessun OLED risponde, controllare alimentazione, GND comune, SDA/SCL e pull-up.
2. Se il PCA9548A non viene trovato, verificare indirizzo `0x70..0x77`, cablaggio SDA/SCL e alimentazione.
3. Se un solo OLED funziona diretto ma non tramite mux, controllare i canali SDx/SCx del PCA.
4. Se gli encoder saltano valori, controllare GND comune, cablaggio A/B e debounce meccanico.
5. Se l'audio non esce, verificare BCK/LRCK/DIN, GND comune, alimentazione DAC e che la UI sia in `WAVEFORM VIEW`.
6. Se l'ESP32 non fa boot con il DAC collegato, provare a scollegare temporaneamente `GPIO15 / LRCK`.

## Librerie Arduino

Richiede le librerie:

- `Wire`
- `Adafruit_GFX`
- `Adafruit_SSD1306`

Su ESP32 vengono usati anche driver/framework disponibili nel core Arduino ESP32:

- `driver/i2s.h`
- `freertos/FreeRTOS.h`
- `freertos/task.h`

## Note di Sviluppo

La struttura e' pensata per restare leggibile mentre il progetto cresce:

- configurazioni e pin stanno in `AppConfig.h`;
- il codice hardware e' separato dal rendering;
- la UI decide cosa succede, il renderer decide come disegnarlo;
- l'audio non viene generato direttamente nel loop principale, ma in un task dedicato;
- i test hardware non sono piu' sketch separati: si attivano con flag.

Questa organizzazione rende piu' semplice cambiare pin, aggiungere modalita', diagnosticare problemi I2C e lavorare sul comportamento del synth senza mescolare tutto dentro un unico `.ino`.
