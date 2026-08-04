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

Il TFT è l'unico display che presenta questi controlli. Dopo il boot mostra
direttamente `Synth View`:

- rotazione encoder master: seleziona `V1`, `V2` o `KICK`;
- click encoder master: play/stop;
- tasto back: apre il menu `Sequencer`;
- `Transport`: Start, Stop e BPM;
- `Pattern`: track, step, nota/hit e step on/rest;
- `Settings`: lunghezza 16/32/48/64, pagina e auto-scroll.

I NeoKey restano il pannello fisico rapido per i 16 step della pagina corrente.
Mostrano una sola corsia alla volta: V1 rosso, V2 blu o kick verde, senza
sovrapporre i colori. OSC3 non è una terza corsia: è il terzo oscillatore della
patch e segue la nota di riferimento della voce corrente. Un evento su V1 o V2
apre tutti e tre gli oscillatori abilitati: le corsie scelgono evento e pitch,
non mettono in solo OSC1 o OSC2.

La nota sotto il playhead viene evidenziata con numero e bordo luminosi. La
pressione di un NeoKey invalida solo griglia e pannelli interessati, così il TFT
non interrompe l'aggiornamento del playhead o dell'auto-scroll.
