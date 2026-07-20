# Legacy four-OLED synthProject

Copia conservata del vecchio sketch Arduino 4 encoder/4 OLED. Non è più il
firmware principale del repository: per la board integrata usare
`../../TheSynthProject/TheSynthProject.ino`.

Questo firmware trasforma un ESP32 in un piccolo pannello interattivo con 4 OLED SSD1306, 4 encoder rotativi, un multiplexer I2C PCA9548A e un DAC I2S PCM5102 / PCM5102A.

## In Breve

- L'Encoder 4 controlla il menu principale.
- Gli schermi sono distribuiti sui canali `0..3` del PCA9548A.
- All'avvio compare uno splash screen distribuito sui quattro display con credits, nome, simbolo glider e loading bar finta.
- La modalita' waveform disegna la forma d'onda e la manda anche in uscita audio I2S.
- La modalita' Pong usa i quattro display come un campo orizzontale largo.
- Il debug seriale puo' essere attivato tenendo premuti insieme Encoder 1 ed Encoder 4.

## File

- `synthProject.ino`: setup, loop, debug runtime e stato condiviso.
- `AppConfig.h`: pin, indirizzi, flag, parametri audio/UI/Pong.
- `Hardware.cpp/.h`: OLED, PCA9548A, I2C, encoder e pulsanti.
- `Render.cpp/.h`: disegno delle schermate.
- `Ui.cpp/.h`: gestione menu e modalita'.
- `Audio.cpp/.h`: generazione audio I2S.
- `Pong.cpp/.h`: logica del mini gioco.

## Layout Fisico

- Schermo 1 sopra Encoder 1
- Schermo 2 sopra Encoder 2
- Schermo 3 sopra Encoder 3
- Schermo 4 sopra Encoder 4

Encoder 4 e' il controller del menu, quindi in modalita' menu il menu principale viene mostrato sullo schermo 4.

## Modalita'

- `ENCODER VIEW`: tutti e quattro gli schermi mostrano il rispettivo encoder in stile contagiri.
- `WAVEFORM VIEW`: editor grafico di waveform con uscita audio I2S su PCM5102.
- `PONG`: mini Pong su campo largo quattro schermi, con paddle su encoder 1 e 4.
- `TEST GENERICO`: diagnostica rapida su bus I2C, PCA9548A, OLED, pin encoder e stato firmware.
- `TEST SPLASH`: anteprima dello splash screen iniziale con loading bar finta.

## Cablaggio Rapido

PCA9548A:

```text
VIN -> ESP32 3V3
GND -> ESP32 GND
SDA -> ESP32 D21
SCL -> ESP32 D22
```

OLED:

```text
OLED 1 -> SD0 / SC0
OLED 2 -> SD1 / SC1
OLED 3 -> SD2 / SC2
OLED 4 -> SD3 / SC3
```

Encoder:

```text
E1: A D13, B D23, SW D14
E2: A D27, B D26, SW D25
E3: A D33, B D32, SW D18
E4: A D19, B RX2, SW TX2
```

PCM5102:

```text
VIN      -> ESP32 3V3
GND      -> ESP32 GND
BCK      -> ESP32 D5
LCK/LRCK -> ESP32 D15
DIN      -> ESP32 D4
SCK      -> GND
```

## Debug

Tenere premuti insieme i pulsanti degli encoder 1 e 4 per circa 1 secondo.

Quando il debug e' attivo:

- parte il Serial Monitor a `115200`;
- vengono stampati heartbeat, valori encoder e click;
- il firmware puo' mostrare diagnostica I2C/PCA/OLED.

## Flag Utili

Nel file `AppConfig.h`:

```cpp
constexpr bool DIRECT_OLED_TEST = false;
constexpr bool PCA_MUX_TEST = false;
constexpr bool MULTI_SCREEN_UI = true;
constexpr bool WIDE_PONG = true;
constexpr unsigned long STARTUP_LOGO_MS = 4000;
constexpr bool DEBUG_SERIAL = false;
```

`DIRECT_OLED_TEST` serve per provare un singolo OLED senza multiplexer. `PCA_MUX_TEST` serve per diagnosticare i canali del PCA9548A. La UI normale usa `MULTI_SCREEN_UI = true`.

## Audio

Il firmware usa I2S stereo a `44100 Hz`. In `WAVEFORM VIEW`, la forma d'onda selezionata viene inviata al DAC; fuori da questa modalita' viene inviato silenzio.

Nota: `D15` e' un pin di strap di boot dell'ESP32. Se dopo aver collegato il DAC la board non parte, provare prima a scollegare `LCK/LRCK`.
