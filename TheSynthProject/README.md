# TheSynthProject

Per i pin ESP32 questo progetto usa esclusivamente i nomi stampati sulla board
della foto. La nomenclatura canonica è in
[`ESP32_PIN_NAMES.md`](ESP32_PIN_NAMES.md).

Per tutti gli altri moduli, nomi e ordine fisico dei pin sono definiti in
[`COMPONENT_PIN_NAMES.md`](COMPONENT_PIN_NAMES.md). I collegamenti descritti
nel progetto devono seguire quell'ordine.

Firmware principale per una ESP32 DevKit V1 / ESP32-WROOM-32. Riunisce:

- TFT ST7735 160x128, encoder master e tasto back;
- 4 OLED SSD1306 128x64 su PCA9548A;
- 4 encoder su MCP23017, sul bus I2C indipendente D19/D15;
- 4 NeoKey 1x4 agli indirizzi `0x30`-`0x33`, per un sequencer a 16 step;
- PCM5102 su I2S;
- synth a due voci, kick, ADSR, filtro, delay, riverbero, preset e sequencer;
- Bluetooth A2DP e schermate di test, incluso un tono I2S diretto a 440 Hz;
- modalità storiche Encoder View, Waveform, Pong, Diagnostics e Splash.

La conversione è pensata per partire dalla vecchia board 4 encoder/4 OLED già
montata. Non occorre smontare PCA, OLED o encoder: seguire la tabella
`vecchio collegamento -> nuovo collegamento` in `WIRING_INTEGRATED.md`.

## Control surface

La modalità predefinita è `SYNTH CONTROL`. Ogni OLED descrive il parametro
dell'encoder sottostante.

- Pressione E1: pagina precedente.
- Pressione E2: pagina successiva.
- Pressione E3: azione contestuale; vedi `CONTROL_SURFACE_ACTIONS.md`.
- Pressione E4: menu principale della control surface.
- E1+E4 tenuti per un secondo: debug seriale control surface.

Pagine synth: Voice 1, Voice 2, Envelope, Mixer, Filter + FX, Sequencer e
Transport. Le modifiche agiscono sullo stesso stato e sullo stesso motore audio
visualizzato dal TFT.

Il sequencer espone solo le funzioni essenziali: play/stop, attivazione, BPM,
voce, step, nota, lunghezza e step on/rest. La riproduzione procede sempre in
avanti e il clock continua anche con volume a zero. I NeoKey applicano insieme
le pressioni simultanee e aggiornano solo i LED realmente cambiati.

Nel menu TFT, `Hardware Test > Control Surface` permette di scegliere con
l'encoder master una delle sei modalità OLED. Control surface e diagnostica
hardware sono raccolte esclusivamente in `Hardware Test`.

`Hardware Test > OLED Debug` funziona anche se tutti gli OLED sono spenti.
Entrando esegue una scansione completa; ruotando l'encoder master la ripete.
Il TFT mostra livelli D21/D22, PCA, indirizzi dei quattro canali, maschere ACK e
INIT e presenza dei NeoKey. Il rapporto esteso viene stampato sul Serial
Monitor a `115200`.

`Hardware Test > MCP Debug` controlla il bus indipendente `D19/D15`, cerca il
MCP23017 a `0x20`, mostra il valore grezzo dei sedici ingressi e aggiorna dal
vivo i contatori E1-E4.

## Modalità storiche preservate

- `ENCODER VIEW`: quadranti originali, pressione E1-E3 per azzerare.
- `WAVEFORM`: Sine/Tri/Step/Const, parametri A/B e uscita PCM5102 a 220 Hz.
- `PONG`: campo largo su quattro OLED; paddle E1 ed E4, partita a 5 punti.
- `DIAGNOSTICS`: stato PCA, OLED, MCP23017, letture e contatori errori.
- `PRELAUNCH`: checklist tecnica di preavvio distribuita sui quattro OLED.

## Build verificata

Verificato con:

- Arduino CLI `1.5.1`;
- ESP32 core `3.3.10`, board `esp32:esp32:esp32`;
- Adafruit GFX `1.12.6`;
- Adafruit ST7735/ST7789 `1.11.0`;
- Adafruit SSD1306 `2.5.17`;
- Adafruit seesaw `1.7.9`;
- audio-tools `1.2.4`;
- ESP32-A2DP `1.8.11`.

La compilazione completa genera correttamente il firmware. La build corrente
usa 1.252.440 byte e entra ancora nella partizione app standard, ma occupa
circa il 95% dello spazio:
in Arduino IDE scegliere `Huge APP (3MB No OTA/1MB SPIFFS)` per lasciare margine
alle prossime integrazioni.

Cablaggio e collaudo: [`WIRING_INTEGRATED.md`](WIRING_INTEGRATED.md).
