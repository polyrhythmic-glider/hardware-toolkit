# Sviluppo del laboratorio hardware

Ogni sketch è un progetto autonomo. Aprire la sua cartella in Arduino IDE e
scegliere la scheda e le librerie indicate nella documentazione del progetto.
I test in [`hardware-tests/`](hardware-tests/) hanno cablaggi propri; leggere i
commenti nello sketch e il README prima del caricamento.

## Neko e display RLCD

Build Arduino senza caricamento sulla scheda:

```sh
./experiments/esp32-e-lcd/NekoAssistant/arduino-build.sh
```

Lo script rileva Arduino CLI oppure usa `ARDUINO_CLI_BIN`, mantiene U8g2 e
output in `.build/neko/` e usa la configurazione ESP32-S3 N16R8 del progetto.
Per il caricamento aggiungere `--upload /dev/ttyACM0`.

## Test host

Servono g++ e Node.js per la verifica della dashboard web.

```sh
./tools/test-host.sh
```

Il comando esegue i test Neko Arduino e quelli degli algoritmi della variante
ESP-IDF, senza collegare schede. I binari host vengono generati in
`.build/tests/neko-idf/`.

Dopo la prima build Arduino sono disponibili anche i test tipografici:

```sh
./experiments/esp32-e-lcd/NekoAssistant/tests/run-typography.sh
```

La verifica degli asset richiede Python e Pillow:

```sh
python3 experiments/esp32-e-lcd/NekoAssistant/tests/test_display_assets.py
```

## Editor e file locali

In VSCodium/VS Code i task inclusi eseguono i test host e la build Arduino Neko.
Gli `.ino` sono associati a C++; clangd può utilizzare un
`compile_commands.json` generato per lo sketch su cui si lavora.
Build, cache, dipendenze scaricate e credenziali locali non vanno committate.
Gli indici precedenti del synth sono conservati localmente in
`.build/archive/synth-index/` e possono essere rigenerati nella nuova repo.

Per compilare, indicizzare o testare il sintetizzatore ESP32 + Daisy usare
[synth-toolkit](https://github.com/polyrhythmic-glider/synth-toolkit).
