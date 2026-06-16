# dueENcoder

Sketch Arduino principale per ESP32 DevKit V1 / ESP32-WROOM-32 con 4 OLED SSD1306 via PCA9548A e 4 encoder rotativi.

## Layout Fisico

- Schermo 1 sopra Encoder 1
- Schermo 2 sopra Encoder 2
- Schermo 3 sopra Encoder 3
- Schermo 4 sopra Encoder 4

Encoder 4 e' il controller del menu, quindi in modalita' menu il menu principale viene mostrato sullo schermo 4.

## Modalita'

- `ENCODER VIEW`: schermi 1-3 in stile contagiri per encoder 1-3; schermo 4 come summary dei click.
- `WAVEFORM VIEW`: editor grafico di waveform con uscita audio I2S su PCM5102.
- `PONG`: mini Pong su campo largo 4 schermi, con paddle su encoder 1 e 4 e velocita' aumentata.

## Debug Runtime

Tenere premuti insieme i pulsanti degli encoder 1 e 4 per circa 1 secondo.

Quando il debug e' attivo:

- parte il Serial Monitor a `115200`
- vengono stampati heartbeat, valori encoder e click
- i pannelli utility possono mostrare diagnostica

Quando il debug e' disattivo, la seriale resta silenziosa.

## Cablaggio Rapido

PCA9548A:

```text
VIN -> ESP32 3V3
GND -> ESP32 GND
SDA -> ESP32 GPIO21
SCL -> ESP32 GPIO22
```

OLED:

```text
OLED 1 -> SD0 / SC0
OLED 2 -> SD1 / SC1
OLED 3 -> SD2 / SC2
OLED 4 -> SD3 / SC3
```

Pull-up consigliate:

```text
3V3 -> SDA con 4.7k
3V3 -> SCL con 4.7k
```

Le pull-up sono fondamentali: sono state la differenza tra schermi corrotti/spenti e setup stabile.

PCM5102:

```text
VIN      -> ESP32 3V3
GND      -> ESP32 GND
BCK      -> ESP32 GPIO5
LCK/LRCK -> ESP32 GPIO15
DIN      -> ESP32 GPIO4
SCK      -> GND
```

Il firmware usa I2S stereo a `44100 Hz`. In `WAVEFORM VIEW`, la forma d'onda selezionata viene anche inviata al DAC. Fuori da questa modalita' viene inviato silenzio.

Nota: `GPIO15` e' un pin di strap di boot dell'ESP32. Se dopo aver collegato il DAC la board non parte, prova prima a scollegare `LCK/LRCK`.
