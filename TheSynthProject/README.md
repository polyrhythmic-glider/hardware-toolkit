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
- Daisy Seed via UART come unico backend audio;
- synth a due voci, kick, due ADSR indipendenti, filtro, delay, riverbero, preset e sequencer;
- schermate hardware e tono audio diretto a 440 Hz;
- modalità storiche Encoder View, Waveform, Pong, Diagnostics e Splash.

La conversione è pensata per partire dalla vecchia board 4 encoder/4 OLED già
montata. Non occorre smontare PCA, OLED o encoder: seguire la tabella
`vecchio collegamento -> nuovo collegamento` in `WIRING_INTEGRATED.md`.

## Control surface

La modalità predefinita è `SYNTH CONTROL`. Ogni OLED descrive il parametro
dell'encoder sottostante.

- Pressione E3: azione contestuale; vedi `CONTROL_SURFACE_ACTIONS.md`.
- Pressione E4: menu principale della control surface.
- Rotazione E4: cambia pagina tra i quattro gruppi compatibili con la traccia TFT attiva.

Il TFT seleziona la traccia V1, V2 o kick. Il `PAGE MENU` degli OLED mostra solo
i quattro gruppi compatibili con quella traccia; le prime tre rotelle modificano
i parametri visualizzati. Nella pagina `OSCILLATOR 1/2`, E1 imposta la nota
assoluta dello step selezionato, E2 Tune ed E3 Fine; Wave, nota risultante e
frequenza sono mostrate nello stesso riepilogo. Ogni step di V1 e V2 conserva
la propria nota indipendente e può essere selezionato direttamente dai NeoKey.

Il sequencer espone solo le funzioni essenziali: play/stop, attivazione, BPM,
voce, step, nota, lunghezza e step on/rest. La riproduzione procede sempre in
avanti e il clock continua anche con volume a zero. I NeoKey aggiornano subito
ogni pressione stabile e aggiornano solo i LED realmente cambiati. La vista LED
è combinata: V1 è rossa, V2 blu e il kick verde; i colori si sommano quando più
corsie usano lo stesso step, mentre la traccia selezionata decide solo cosa si
modifica.

Nel menu TFT, `Utility > Control Surface` permette di scegliere con
l'encoder master una delle sei modalità OLED. Control surface e diagnostica
hardware sono raccolte in `Utility`.

`Utility > OLED Debug` funziona anche se tutti gli OLED sono spenti.
Entrando esegue una scansione completa; ruotando l'encoder master la ripete.
Il TFT mostra livelli D21/D22, PCA, indirizzi dei quattro canali, maschere ACK e
INIT e presenza dei NeoKey. Il rapporto esteso viene stampato sul Serial
Monitor a `115200`.

`Utility > MCP Debug` controlla il bus indipendente `D19/D15`, cerca il
MCP23017 a `0x20`, mostra il valore grezzo dei sedici ingressi e aggiorna dal
vivo i contatori E1-E4.

Durante il boot il TFT conserva lo splash `Polyrhythmic Glider` e la barra di
caricamento. La checklist di ESP32/TFT, PCA+OLED, quattro NeoKey,
MCP23017+encoder e delle due direzioni UART `ESP > DAISY` e `DAISY > ESP`
viene stampata sul Monitor Seriale. La pagina resta disponibile su richiesta
in `Utility > Connection Status`; ruotando l'encoder master vengono ripetute
le scansioni OLED/PCA e MCP. `ACK` e `RX` sono contatori reali dei pacchetti,
non semplici indicatori configurati a priori.

## Inviluppi e modulazione

L'area `Envelope` espone `ENV1` e `ENV2`, ciascuno con Attack, Decay, Sustain,
Release e un sottomenu `Destinations`. Ogni destinazione ha `Enabled` e un
amount bipolare da `-100%` a `+100%`; più route possono essere attive nello
stesso momento. ENV1 è assegnato di default ad `Amp Level` al `+100%`, mentre
ENV2 parte libero.

La configurazione è memorizzata come tabella generica `source x destination`,
separata sia dai menu sia dai processor DSP. Per aggiungere in futuro LFO,
Velocity, Mod Wheel o Aftertouch non occorre cambiare le route esistenti: va
aggiunto soltanto il producer della nuova sorgente. I preset v6 salvano note
assolute, entrambi gli ADSR e tutte le route; i preset v3/v4/v5 vengono migrati
mantenendo il risultato sonoro precedente e ENV1 sul livello amp.

`Utility > Serial Diagnostics` e' attivato per default durante il collaudo.
All'avvio stampa un report completo di sistema, OLED, NeoKey, MCP23017 e link
Daisy; in seguito la telemetria del sequencer e' limitata a una riga ogni
750 ms e quella audio/sistema/UART a una riga ogni 2 s. La riga
`[DAISY][LINK]` mostra separatamente `ESP->Daisy`, `Daisy->ESP`, pacchetti
trasmessi/scartati, ricevuti/non validi, i contatori accettati/rifiutati dalla
Daisy e i riavvii/errori del suo ricevitore. Se la ESP32 viene riavviata, la
Daisy riattiva automaticamente la ricezione UART senza richiedere un proprio
power cycle. Dal menu la diagnostica seriale puo' essere disattivata durante
l'uso normale.

La ESP32 invia i controlli via UART; audio, sequencer e DSP sono eseguiti
esclusivamente dalla Daisy. Il test a 440 Hz bypassa sequencer e patch synth.

Durante il boot i quattro framebuffer OLED vengono inviati una sola volta. In
esecuzione il playhead non invalida le pagine che non lo mostrano; nella vista
Synth il TFT aggiorna soltanto numero di step, nota, frequenza e celle della
griglia. Il monitor ingressi e' limitato a 10 frame/s e i quattro NeoKey sono
letti ogni 10 ms, evitando traffico display/I2C continuo senza rallentare i
controlli.

## Modalità storiche preservate

- `ENCODER VIEW`: quadranti originali, pressione E1-E3 per azzerare.
- `WAVEFORM`: visualizzatore storico Sine/Tri/Step/Const con parametri A/B, senza uscita audio.
- `PONG`: campo largo su quattro OLED; paddle E1 ed E4, partita a 5 punti.
- `DIAGNOSTICS`: stato PCA, OLED, MCP23017, letture e contatori errori.
- `PRELAUNCH`: checklist tecnica di preavvio distribuita sui quattro OLED.

## Build verificata

Verificato con:

- Arduino CLI `1.5.0`;
- ESP32 core `3.3.11`, board `esp32:esp32:esp32`;
- Adafruit GFX `1.12.6`;
- Adafruit ST7735/ST7789 `1.11.0`;
- Adafruit SSD1306 `2.5.17`;
- Adafruit seesaw `1.7.9`.

La build Daisy-only usa 416.272 byte su 1.310.720 (31%) e 26.292 byte di RAM
globale su 327.680 (8%).

Cablaggio e collaudo: [`WIRING_INTEGRATED.md`](WIRING_INTEGRATED.md).

Migrazione e collegamento Daisy Seed:
[`DAISY_MIGRATION.md`](DAISY_MIGRATION.md).
