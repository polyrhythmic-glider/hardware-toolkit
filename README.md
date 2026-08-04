# Hardware Toolkit

Il repository ora ha un solo firmware principale per ESP32:

- [`TheSynthProject/`](TheSynthProject/) contiene l'applicazione integrata da compilare e caricare;
- [`examples-and-tests/`](examples-and-tests/) contiene sketch diagnostici, il vecchio firmware 4 OLED e i progetti e-LCD separati.

`TheSynthProject` unisce il pannello TFT ST7735 con encoder singolo e tasto back alla
control surface originale con 4 encoder, 3 OLED SH1106, 1 OLED SSD1306,
PCA9548A e Pong. La
ESP32 gestisce interfaccia, preset e pattern; la Daisy Seed esegue il clock
audio, il sequencer e il DSP. La Daisy è l'unico backend e l'unica uscita audio.

La scheda 4 encoder/4 OLED già montata è la base consigliata: PCA, OLED e
meccanica restano invariati. La tabella di migrazione indica quali fili encoder
spostare e come collegare la UART verso la Daisy.

## Da dove iniziare

1. Leggere [`TheSynthProject/WIRING_INTEGRATED.md`](TheSynthProject/WIRING_INTEGRATED.md).
2. Aprire [`TheSynthProject/TheSynthProject.ino`](TheSynthProject/TheSynthProject.ino) con Arduino IDE.
3. Selezionare `ESP32 Dev Module` per una ESP32 DevKit V1 / WROOM-32.
4. Compilare, caricare e seguire la sequenza di collaudo nel documento di cablaggio.

Il vecchio setup autonomo 4 encoder/4 OLED è conservato senza modifiche in
[`examples-and-tests/legacy-four-oled/`](examples-and-tests/legacy-four-oled/).

Il firmware audio Daisy Seed è in
[`DaisySynth/`](DaisySynth/). Cablaggio UART, uscita audio e collaudo sono
descritti in
[`TheSynthProject/DAISY_MIGRATION.md`](TheSynthProject/DAISY_MIGRATION.md).

## VSCodium e clangd

Per indicizzare correttamente sia le librerie Arduino/ESP32 sia
libDaisy/DaisySP, generare il database unificato:

```sh
./tools/generate_compile_commands.sh
```

Configurazione editor, task VSCodium e risoluzione degli errori come
`Unknown type name 'DaisySeed'` sono descritti in
[`DEVELOPMENT.md`](DEVELOPMENT.md).
