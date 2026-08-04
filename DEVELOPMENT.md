# VSCodium, clangd e librerie embedded

Il repository contiene due toolchain differenti:

- `TheSynthProject`: Arduino ESP32, core ESP32 e librerie Adafruit;
- `DaisySynth`: `arm-none-eabi-g++`, libDaisy e DaisySP.

Un elenco manuale di `includePath` non basta: clangd deve ricevere anche target,
define e flag del compilatore. Lo script seguente genera un unico compilation
database usando i comandi reali dei due sistemi di build:

```sh
./tools/generate_compile_commands.sh
```

In VSCodium lo stesso comando è disponibile da `Terminal > Run Task` come
`clangd: rigenera database ESP32 + Daisy`. Al termine eseguire dalla Command
Palette `clangd: Restart language server`; in alternativa chiudere e riaprire
il workspace.

Il file generato `compile_commands.json` e la directory `.clangd-build/` sono
locali e ignorati da Git. Vanno rigenerati dopo aggiornamenti del core ESP32,
delle librerie Arduino, di libDaisy/DaisySP o dei relativi percorsi.

## Percorsi non standard

Lo script rileva `arduino-cli` nel `PATH` oppure dentro Arduino IDE per Linux.
I percorsi possono essere specificati esplicitamente:

```sh
ARDUINO_CLI_BIN=/percorso/arduino-cli \
LIBDAISY_DIR=/percorso/libDaisy \
DAISYSP_DIR=/percorso/DaisySP \
./tools/generate_compile_commands.sh
```

La configurazione VSCodium associa `*.ino` a C++, abilita l'indice in
background e autorizza clangd a interrogare sia `arm-none-eabi-g++` sia il
compilatore Xtensa ESP32. Questo permette di risolvere correttamente simboli come
`DaisySeed`, gli header `daisy_seed.h`/`daisysp.h`, Arduino e le librerie
Adafruit senza aggiungere percorsi globali all'editor.
