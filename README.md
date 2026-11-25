# Hardware Toolkit

Laboratorio hardware di Polyrhythmic Glider: sketch diagnostici, esperimenti
ESP32/Arduino e firmware storici.

Il sintetizzatore integrato **ESP32 + Daisy Seed** è stato trasferito in
[**synth-toolkit**](https://github.com/polyrhythmic-glider/synth-toolkit), con firmware,
cablaggi, protocollo UART, DSP, test specifici e configurazione di sviluppo.

## Progetti

| Percorso | Uso |
| --- | --- |
| [`hardware-tests/`](hardware-tests/) | Sketch indipendenti per I2C, MCP23017, NeoKey, LED e matrice |
| [`experiments/esp32-e-lcd/`](experiments/esp32-e-lcd/) | Waveshare ESP32-S3 RLCD, Neko Arduino, variante ESP-IDF ed explorer ST7305 |
| [`archive/synthProject/`](archive/synthProject/) | Vecchio pannello quattro OLED con audio I2S PCM5102 e Pong |
| [`tools/`](tools/) | Verifiche host dei progetti del laboratorio |

Per Neko usare lo [sketch Arduino](experiments/esp32-e-lcd/NekoAssistant/).
La [variante ESP-IDF](experiments/esp32-e-lcd/neko-assistant/) resta un esperimento
separato, con requisiti e componenti esterni documentati nel suo README.

## Sviluppo e test

Istruzioni di compilazione, test host e uso dell'editor sono in
[`DEVELOPMENT.md`](DEVELOPMENT.md). Gli sketch Arduino sono raccolti in cartelle
con lo stesso nome del file `.ino`, così si aprono direttamente in Arduino IDE.

```sh
./tools/test-host.sh
```

Cache, firmware compilati e backup locali restano fuori da Git in `.build/`.

## Vecchi percorsi

La cronologia precedente alla separazione è conservata in questa repository.

| Prima | Adesso |
| --- | --- |
| `TheSynthProject/`, `DaisySynth/`, riferimenti Daisy e strumenti clangd synth | [synth-toolkit](https://github.com/polyrhythmic-glider/synth-toolkit) |
| `examples-and-tests/hardware-tests/` | [`hardware-tests/`](hardware-tests/) |
| `examples-and-tests/esp32-e-lcd/` | [`experiments/esp32-e-lcd/`](experiments/esp32-e-lcd/) |
| `examples-and-tests/legacy-four-oled/` | [`archive/synthProject/`](archive/synthProject/) |
