# Test hardware autonomi

Aprire lo sketch nella propria cartella con Arduino IDE. Ogni test usa solo
le periferiche indicate nel codice e non dipende dal firmware synth.

| Sketch | Scopo |
| --- | --- |
| [`i2cScannerD21D19/`](i2cScannerD21D19/) | Scanner I2C ESP32 sul bus D21/D19 |
| [`mcp23017_single_encoder_test/`](mcp23017_single_encoder_test/) | Singolo encoder tramite espansore MCP23017; [cablaggio](mcp23017_single_encoder_test/README.md) |
| [`neokey1x4DualTest/`](neokey1x4DualTest/) | Due moduli NeoKey 1x4 sullo stesso bus |
| [`neokeySetSecondAddress031/`](neokeySetSecondAddress031/) | Configurazione del secondo NeoKey a indirizzo 0x31 |
| [`ledTest/`](ledTest/) | LED con 74HC595, matrice MAX7219 e microfono KY-037; pin Arduino definiti nello sketch |

Per la control surface integrata ESP32 + Daisy seguire il cablaggio di
[synth-toolkit](https://github.com/polyrhythmic-glider/synth-toolkit).
