# Control Surface Actions

I quattro OLED sono una control surface performativa globale. Non seguono la
traccia selezionata sul TFT e non contengono pagine di sequencer o transport.
E1, E2 ed E3 modificano i tre parametri mostrati. E4 ruota fra 15 pagine;
un click entra nelle tre sottopagine della pagina corrente e un altro click
torna alla selezione delle pagine. Ogni pagina ricorda la propria sottopagina.
Il quarto OLED mostra quattro nomi e tre pallini per ogni riga.

| Pagine OLED | Sottopagina 1: E1 / E2 / E3 | Sottopagina 2: E1 / E2 / E3 | Sottopagina 3: E1 / E2 / E3 | Click E3 |
| --- | --- | --- | --- | --- |
| `V1 OSC 1-3`, `V2 OSC 1-3` (sei pagine) | Wave / Level / Fold | Octave / Tune / Detune | Phase / Pulse / Pan | On/Off oscillatore |
| `ENV 1-3` (tre pagine) | Attack / Decay o Sustain / Release | Amount / Velocity / Retrigger | Destination / Route On / Route Amount | On/Off inviluppo |
| `LFO 1-2` (due pagine) | Rate / Depth / Destination | Wave / Phase / Bipolar | Retrigger / Free Run / Tempo Sync | On/Off LFO |
| `PERF` | Master / Tune / Tempo | V1 Level / V2 Level / 909 Level | V1 O1 Pan / V1 O2 Pan / V1 O3 Pan | - |
| `FILTER` | Cutoff / Resonance / Type | Noise / Pan / Filter On | Source / Destination / Amount | On/Off filtro |
| `FX` | Delay Mix / Time / Feedback | Reverb Mix / Room / Delay Send | Source / Destination / Amount | - |
| `909 BD` | Level / Tune / Decay | Attack / Sweep / Tune | Drive / Level / Attack | Trigger kick |

Il click E2 nella sottopagina ADSR alterna Decay e Sustain. Le quantità nelle
rotte FILTER e FX abilitano la rotta quando vengono modificate. Tempo Sync
degli LFO resta un parametro riservato, senza sincronizzazione DSP attiva.

Tenere premuto E4 per 700 ms apre il selettore delle modalità OLED. Le modalità
`Encoder View`, `Waveform`, `Pong`, `Diagnostics` e `Prelaunch` restano
disponibili; tornando a `Synth Control` si ritrova la banca performativa.

## Sequencer e transport

Il TFT è l'unico display che presenta questi controlli. Il boot mostra
`DAISY PAIRING` nella checklist e rimane fermo se Daisy non è pronta. Dopo
l'avvio, la Synth View presenta direttamente i controlli del sequencer:

- rotazione encoder master: seleziona `V1`, `V2` o `KICK`;
- click encoder master: play/stop;
- tasto back: apre il menu `Sequencer`;
- `Transport`: Start, Stop e BPM;
- `Pattern`: editor unico con traccia, step, nota e stato on/rest sempre visibili;
- `Settings`: lunghezza 16/32/48/64, pagina e auto-scroll.

In `Pattern` il primo step della pagina è già selezionato:

- NeoKey diverso: seleziona lo step senza cambiarne lo stato;
- NeoKey dello step selezionato: alterna nota e pausa;
- rotazione encoder master: cambia la nota e attiva lo step (su KICK, destra = hit e sinistra = pausa);
- click encoder master: passa fra V1, V2 e KICK;
- Back: torna al menu Sequencer.

Durante la modifica la pagina resta ferma. Per modificare gli step 17–64,
scegli la pagina in `Settings > Seq Page`, poi riapri Pattern. Se il transport
è in esecuzione, disattiva `Page Auto Scroll` per mantenere la pagina scelta.
Uscendo da Pattern si ripristina il normale comportamento dell'auto-scroll.

Fuori da Pattern, i NeoKey restano il pannello fisico rapido per i 16 step della pagina corrente.
Mostrano una sola corsia alla volta: V1 rosso, V2 blu o kick verde, senza
sovrapporre i colori. V1 e V2 possiedono ciascuna tre oscillatori indipendenti;
un evento apre gli oscillatori abilitati del proprio banco sulla nota della
traccia. Una pausa chiude soltanto il gate della propria voce. OSC3 è il terzo
oscillatore del banco, senza una corsia aggiuntiva.

La nota sotto il playhead viene evidenziata con numero e bordo luminosi. La
pressione di un NeoKey invalida solo griglia e pannelli interessati, così il TFT
non interrompe l'aggiornamento del playhead o dell'auto-scroll.
