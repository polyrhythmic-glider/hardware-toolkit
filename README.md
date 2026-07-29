# Hardware Toolkit

Il repository ora ha un solo firmware principale per ESP32:

- [`TheSynthProject/`](TheSynthProject/) contiene l'applicazione integrata da compilare e caricare;
- [`examples-and-tests/`](examples-and-tests/) contiene sketch diagnostici, il vecchio firmware 4 OLED e i progetti e-LCD separati.

`TheSynthProject` unisce il pannello TFT ST7735 con encoder singolo e tasto back alla
control surface originale con 4 encoder, 4 OLED SSD1306, PCA9548A e Pong. Il
motore audio nativo ESP32, il sequencer essenziale, Bluetooth, NeoKey, preset e le
schermate TFT restano nel firmware principale.

La scheda 4 encoder/4 OLED già montata è la base consigliata: PCA, OLED e
meccanica restano invariati. La tabella di migrazione indica soltanto quali fili
encoder e PCM spostare per liberare i pin del TFT.

## Da dove iniziare

1. Leggere [`TheSynthProject/WIRING_INTEGRATED.md`](TheSynthProject/WIRING_INTEGRATED.md).
2. Aprire [`TheSynthProject/TheSynthProject.ino`](TheSynthProject/TheSynthProject.ino) con Arduino IDE.
3. Selezionare `ESP32 Dev Module` per una ESP32 DevKit V1 / WROOM-32.
4. Selezionare `Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)`.
5. Compilare, caricare e seguire la sequenza di collaudo nel documento di cablaggio.

Il vecchio setup autonomo 4 encoder/4 OLED è conservato senza modifiche in
[`examples-and-tests/legacy-four-oled/`](examples-and-tests/legacy-four-oled/).
