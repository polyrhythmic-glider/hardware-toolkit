# Control Surface Actions

I quattro OLED sono una control surface performativa globale. Non seguono la
traccia selezionata sul TFT e non contengono pagine di sequencer o transport.
E1, E2 ed E3 modificano i tre parametri mostrati; E4 ruota fra tutte le pagine
e il quarto OLED mostra una finestra scorrevole di quattro nomi.

| Pagina OLED | E1 | E2 | E3 | Click E3 |
| --- | --- | --- | --- | --- |
| `OSCILLATOR 1` | Wave | Level | Fold | On/Off OSC1 |
| `OSCILLATOR 2` | Wave | Level | Fold | On/Off OSC2 |
| `OSCILLATOR 3` | Wave | Level | Fold | On/Off OSC3 |
| `ENVELOPE 1` | Attack | Decay | Release | On/Off ENV1 |
| `ENVELOPE 2` | Attack | Decay | Release | On/Off ENV2 |
| `ENVELOPE 3` | Attack | Decay | Release | On/Off ENV3 |
| `LFO 1` | Rate | Depth | Destination | On/Off LFO1 |
| `LFO 2` | Rate | Depth | Destination | On/Off LFO2 |
| `PERFORMANCE` | Master | Noise | Pan | - |
| `FILTER` | Cutoff | Resonance | LP/HP/BP | On/Off filter |
| `DELAY + REV` | Delay mix | Feedback | Reverb mix | - |
| `909 MAIN` | Level | Tune | Attack | Trigger kick |
| `909 SHAPE` | Decay | Sweep | Drive | Trigger kick |

Il click E4 apre il selettore delle modalità OLED storiche. Le modalità
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
sovrapporre i colori. OSC3 non è una terza corsia: è il terzo oscillatore della
patch e segue la nota di riferimento della voce corrente. Un evento su V1 o V2
apre tutti e tre gli oscillatori abilitati: le corsie scelgono evento e pitch,
non mettono in solo OSC1 o OSC2.

La nota sotto il playhead viene evidenziata con numero e bordo luminosi. La
pressione di un NeoKey invalida solo griglia e pannelli interessati, così il TFT
non interrompe l'aggiornamento del playhead o dell'auto-scroll.
