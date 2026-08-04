# TheSynthDaisy

Firmware audio per Daisy Seed usato da `TheSynthProject` sulla ESP32.

La Daisy riceve via UART snapshot completi dello stato del synth, li applica
solo ai confini dei blocchi audio e mantiene sequencer e DSP nel callback
realtime. Il parsing UART e la telemetria restano fuori dal callback.
Il ricevitore si riavvia automaticamente dopo errori UART o un power cycle
della ESP32; i contatori di recupero tornano alla ESP32 nella telemetria.
Il kick usa `DaisySP::SyntheticBassDrum`, il modello sintetico 909-style della
libreria. ESP32 controlla Level, Tune, Attack, Decay, Sweep e Drive. Il percorso
dry resta a livello pieno: `Drive 0%` e' pulito, mentre valori superiori
miscelano progressivamente la saturazione senza essere necessari per rendere
udibile la voce.
Il controllo `Utility > Output Volume` della ESP32 viene applicato alla fine
della catena stereo e regola insieme voci, kick, effetti e test audio.

Il synth interno e' ora organizzato come quattro `Voice` indipendenti. Ogni
voce contiene tre oscillatori, noise, filtro multimode, VCA, tre ADSR, due LFO
e una modulation matrix fixed-size. UART v4 riceve il patch moderno completo;
il compatibility layer conserva le due track melodiche e il kick del
sequencer, mentre OSC3 segue la nota di riferimento della voce corrente.
I preset sono gestiti dalla ESP32 nel formato v8 con migrazione da v3-v7.

Le 16 forme mostrate dalla UI sono tutte implementate nel DSP. Quando OSC1,
OSC2 e OSC3 sono abilitati e hanno livello maggiore di zero, ogni evento
melodico del sequencer li apre insieme dentro la stessa voce; le due lane V1/V2
restano lane di evento/pitch e non interruttori esclusivi dei rispettivi OSC.

Architettura, routing, allocazione, limiti e validazione realtime:
[`DSP_ARCHITECTURE.md`](DSP_ARCHITECTURE.md).

Versioni preset e default di compatibilita':
[`../TheSynthProject/PRESET_FORMAT.md`](../TheSynthProject/PRESET_FORMAT.md).

Istruzioni complete, pinout e sequenza di collaudo:
[`../TheSynthProject/DAISY_MIGRATION.md`](../TheSynthProject/DAISY_MIGRATION.md).

Build rapida:

```sh
make
make program-dfu
```

Il target prodotto è `build/TheSynthDaisy.bin`. Il riverbero usa il modulo
`ReverbSc` di DaisySP-LGPL; per questo il `Makefile` abilita esplicitamente
`USE_DAISYSP_LGPL`.

Il protocollo è condiviso con la ESP32. Quando cambia
`../TheSynthProject/DaisyLinkProtocol.h`, caricare entrambi i firmware: versioni
diverse rifiutano intenzionalmente i pacchetti.
