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
- 4 NeoKey 1x4 agli indirizzi `0x30`-`0x33`, per una pagina fisica da 16 step;
- Daisy Seed via UART come unico backend audio;
- control surface a due track su motore Daisy polifonico a quattro voci, kick,
  filtro, delay, riverbero, preset e sequencer;
- schermate hardware e tono audio diretto a 440 Hz;
- modalità storiche Encoder View, Waveform, Pong, Diagnostics e Splash.

La conversione è pensata per partire dalla vecchia board 4 encoder/4 OLED già
montata. Non occorre smontare PCA, OLED o encoder: seguire la tabella
`vecchio collegamento -> nuovo collegamento` in `WIRING_INTEGRATED.md`.

## Control surface

La modalità predefinita è `SYNTH CONTROL`. Ogni OLED descrive il parametro
dell'encoder sottostante.

- Pressione E3: azione contestuale; vedi `CONTROL_SURFACE_ACTIONS.md`.
- Click E4: alterna selezione delle pagine e delle sottopagine.
- Pressione E4 per 700 ms: menu principale della control surface.
- Rotazione E4: scorre 15 pagine oppure le tre sottopagine della pagina corrente.

Gli OLED non seguono più la traccia del sequencer e non mostrano transport o
step. Le pagine sono OSC1-3 di V1, OSC1-3 di V2, ENV1-3, LFO1-2, PERF, FILTER,
FX e 909 BD. E1-E3 modificano sempre i tre parametri visibili; il click
E3 abilita/disabilita il blocco corrente oppure triggera il kick. Il quarto OLED
mostra una finestra scorrevole di quattro pagine.

Durante il loading, la checklist include `DAISY PAIRING`. Se la comunicazione
non è pronta, il boot resta sulla checklist e mostra `waiting daisy :)` sotto
la barra di caricamento; la ricerca continua automaticamente. Quando Daisy è
pronta, il TFT apre la Synth View; il tasto back mostra il
menu raggruppato in `Transport`, `Pattern` e `Settings`. Start/Stop sono
l'unico controllo di accensione del sequencer: il precedente `Sequencer Power`,
ridondante, è stato rimosso. La lunghezza si sceglie tra 16, 32, 48 e 64 step,
organizzati in quattro pagine fisiche da 16. Quattro indicatori a pallino sono
visibili nella Synth View e in tutte le schermate del sequencer. `Page Auto
Scroll` decide se la pagina segue il playhead o resta bloccata su quella scelta
manualmente.

La verifica richiede tre conferme di snapshot distinti realmente inviati,
applicati dalla Daisy e restituiti nella telemetria. Il timeout è di 500 ms
senza stati validi o senza avanzamento delle conferme. La UART ESP resta aperta
durante tutta la sessione; Daisy riabilita automaticamente il proprio ricevitore
se non riceve un frame valido al boot o dopo un errore. I parser recuperano anche
frame troncati. Il protocollo è v5 e **richiede l'aggiornamento di entrambi i
firmware**: i pacchetti di versioni precedenti vengono rifiutati.

Test della logica di connessione: `make -C DaisySynth test-link-host` dalla
radice del repository. Collaudo sulle schede dopo il caricamento:

1. Accendere ESP32 da sola: finito il loading deve restare sulla checklist con
   `waiting daisy :)`; accendere Daisy dopo almeno 10 secondi e verificare
   l'ingresso automatico in Synth View.
2. Ripetere accendendo prima Daisy e successivamente ESP32.
3. Dopo l'avvio, scollegare e ripristinare Daisy: il link deve ristabilirsi
   automaticamente senza riavviare ESP32.
4. Interrompere e ripristinare separatamente ciascuna linea UART: anche la
   perdita di una sola direzione deve causare attesa e successivo recupero.
5. Ripetere più cicli, inclusa una disconnessione durante l'editor Pattern,
   verificando che la selezione dello step venga conservata.

La riproduzione procede sempre in avanti e il clock continua anche con volume a
zero. I NeoKey aggiornano subito ogni pressione stabile e soltanto i LED
realmente cambiati. Mostrano esclusivamente la pagina e la traccia selezionate:
V1 è rossa, V2 blu e il kick verde, senza sovrapposizione o somma dei colori. La
schermata `Pattern` permette di scegliere lo step con un NeoKey e cambiarne
la nota ruotando l'encoder master. Ripremere il NeoKey selezionato alterna nota
e pausa; il click dell'encoder cambia traccia e Back torna al menu. Entrando è
già selezionato il primo step della pagina. Durante la modifica l'auto-scroll
è sospeso; la pagina si sceglie in `Settings > Seq Page`.

Nella griglia TFT la cella suonata conserva numero e bordo luminosi anche se
contiene una nota. Inserire o rimuovere uno step ridisegna soltanto le regioni
interessate: non viene più eseguito un `fillScreen()` ad ogni pressione, quindi
playhead e auto-scroll possono continuare a seguire la telemetria Daisy.

La traccia kick pilota il modello `SyntheticBassDrum` di DaisySP, scelto per il
carattere 909. Nel `PAGE MENU` ogni oggetto ha una sola riga e tre pallini:

| Pagina | Sottopagina 1 | Sottopagina 2 | Sottopagina 3 |
| --- | --- | --- | --- |
| V1 OSC 1-3, V2 OSC 1-3 (sei righe separate) | base | pitch | shape |
| ENV 1, ENV 2, ENV 3 (righe separate) | ADSR | dynamics | modulation |
| LFO 1, LFO 2 (righe separate) | motion | shape | behaviour |
| PERF | master | livelli | stereo |
| FILTER | cutoff-res-type | noise-pan | modulation |
| FX | delay | reverb | routing |
| 909 BD | body | transient | drive |

Ruotare E4 seleziona una riga; il click entra nella pagina e da quel momento
la rotazione di E4 cambia soltanto la sua sottopagina. Un altro click torna
alla selezione delle righe. Il pallino pieno indica la sottopagina corrente,
ricordata separatamente per ogni oggetto; il titolo dell'OLED E4 ne mostra il nome.
E1-E3 modificano i parametri della sottopagina. In ADSR E1 regola attack,
E2 decay oppure sustain (click E2 per alternarli), E3 release.
Il click E3 mantiene l'azione della pagina: on/off per OSC, ENV, LFO e FILTER,
trigger per 909 BD.

Le pagine modulation degli ENV selezionano destinazione, abilitazione e quantità
della singola rotta. FILTER/modulation e FX/routing espongono sorgente,
destinazione e quantità delle rotte di modulazione già disponibili: cutoff o
resonance per FILTER, delay mix o reverb mix per FX. Modificare la quantità
abilita la rotta selezionata; zero ne annulla il contributo.

909 BD/body regola Level, Tune e Decay; transient regola Attack, Sweep e Tune;
drive regola Drive, Level e Attack. I preset precedenti restano leggibili:
il vecchio valore `Click` viene interpretato come `Attack` nello stesso campo salvato.

Nel menu TFT, `Utility > Control Surface` permette di scegliere con
l'encoder master una delle sei modalità OLED. Control surface e diagnostica
hardware sono raccolte in `Utility`.

`Test Audio` compare soltanto in `Utility`, non in `Wave Engine`.

`Utility > Output Volume` controlla il livello stereo finale della Daisy,
inclusi voci, kick, effetti e tono di test. Il valore iniziale e' `70%`, puo'
essere regolato tra `0%` e `100%` ed e' incluso nei preset.

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

`Wave Engine` sul TFT espone OSC1, OSC2 e OSC3 del banco V1; gli OLED consentono
di modificare entrambi i banchi V1 e V2. L'area `Envelope` espone ENV1, ENV2 ed
ENV3, ciascuno con Attack, Decay, Sustain, Release e un sottomenu
`Destinations`. Ogni destinazione ha `Enabled` e un amount bipolare da `-100%`
a `+100%`; più route possono essere attive nello stesso momento. Le
destinazioni corrispondono al DSP moderno: Amp, filtro, pitch/level/PWM/fold di
ognuno dei tre oscillatori, all-osc pitch, noise, pan, delay e reverb.

ENV1 è assegnato di default ad Amp al `+100%`; ENV2 ed ENV3 partono disabilitati
e senza route. LFO1 e LFO2 partono disabilitati e si regolano dagli OLED con
Rate, Depth e Destination. La configurazione è memorizzata nella tabella
generica `source x destination`, salvata nei preset v9 e trasmessa con UART v5.
I preset v3-v8 vengono migrati nei due banchi indipendenti; per v3-v7, OSC3,
ENV3 e LFO sono inizializzati in modo neutro. Vedi [`PRESET_FORMAT.md`](PRESET_FORMAT.md).

Ogni sottomenu OSC del TFT contiene Power, Wave, Level, Octave, Tune, Detune,
Phase, Pulse Width, Fold, Pan, Key Tracking, Mod Input e Scope. Le 16 forme
visibili sono tutte renderizzate dalla Daisy, incluse `EVEN`, `ODD` e `FORMNT`.
Se i tre OSC sono `ON` e hanno Level maggiore di zero, uno step melodico li
suona contemporaneamente nella propria voce. V1 e V2 hanno note, gate, livelli
e tre oscillatori indipendenti: una pausa o una modifica su V1 non cambia V2.
I parametri di inviluppi, LFO, filtro e modulazione restano comuni ai due banchi,
con stato DSP separato per ogni voce.

`Utility > Serial Diagnostics` e' disattivato per default. All'avvio il Monitor
Seriale stampa una sola mappa ASCII con TFT, OLED, NeoKey, MCP23017 e le due
direzioni del link Daisy. Attivando la diagnostica dal menu, la telemetria del
sequencer e' limitata a una riga ogni 750 ms e quella audio/sistema/UART a una
riga ogni 2 s. La riga
`[DAISY][LINK]` mostra separatamente `ESP->Daisy`, `Daisy->ESP`, pacchetti
trasmessi/scartati, ricevuti/non validi, i contatori accettati/rifiutati dalla
Daisy e i riavvii/errori del suo ricevitore. Se la ESP32 viene riavviata, la
Daisy riattiva automaticamente la ricezione UART senza richiedere un proprio
power cycle. La diagnostica puo' essere nuovamente disattivata dallo stesso
menu per tornare al funzionamento silenzioso.

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

La build ESP32 usa 423.116 byte su 1.310.720 (32%) e 27.156 byte di RAM globale
su 327.680 (8%).

Cablaggio e collaudo: [`WIRING_INTEGRATED.md`](WIRING_INTEGRATED.md).

Migrazione e collegamento Daisy Seed:
[`DAISY_MIGRATION.md`](DAISY_MIGRATION.md).
