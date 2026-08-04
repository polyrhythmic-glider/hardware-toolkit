# TheSynthDaisy

Firmware audio per Daisy Seed usato da `TheSynthProject` sulla ESP32.

La Daisy riceve via UART snapshot completi dello stato del synth, li applica
solo ai confini dei blocchi audio e mantiene sequencer e DSP nel callback
realtime. Il parsing UART e la telemetria restano fuori dal callback.
Il ricevitore si riavvia automaticamente dopo errori UART o un power cycle
della ESP32; i contatori di recupero tornano alla ESP32 nella telemetria.
Il kick mantiene un percorso dry a livello pieno: `Drive 0%` e' pulito, mentre
valori superiori miscelano progressivamente la saturazione senza essere
necessari per rendere udibile la voce.

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
