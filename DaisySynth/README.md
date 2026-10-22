# TheSynthDaisy

Firmware audio per Daisy Seed usato da `TheSynthProject` sulla ESP32.

La Daisy riceve via UART snapshot completi dello stato del synth, li applica
solo ai confini dei blocchi audio e mantiene sequencer e DSP nel callback
realtime. Il parsing UART e la telemetria restano fuori dal callback.
Il ricevitore si riavvia automaticamente dopo errori UART o un power cycle
della ESP32; i contatori di recupero tornano alla ESP32 nella telemetria.
Il recupero azzera i frame parziali e limita la frequenza dei tentativi;
vengono controllati anche gli errori HAL non notificati dal callback DMA.
La ESP32 attende tre conferme di snapshot applicati prima di aprire la UI e
mostra `Reconnecting...` se il collegamento si interrompe. Per usare il nuovo
avvio e il recupero completo aggiornare **sia ESP32 sia Daisy**.
Test del protocollo e della connessione: `make test-link-host`.

Per isolare la UART dal DSP quando ESP32 non riceve stati validi:
`make link-diagnostic` prepara un firmware **senza audio**, nella cartella
separata `build-link-diagnostic`. Mettere Daisy in DFU, poi eseguire
`make program-link-diagnostic` per caricarlo. Il LED lampeggia se il ciclo
principale funziona ma non arrivano controlli validi; rimane acceso quando li
riceve. ESP32 dovrebbe superare l'attesa dopo le conferme UART. Se questa
versione funziona e quella normale no, il guasto dipende dal percorso
DSP/audio o dal suo carico, non dalla sola logica di attesa. Per tornare al
firmware audio, rientrare in DFU ed eseguire `make && make program-dfu`.
Il kick usa `DaisySP::SyntheticBassDrum`, il modello sintetico 909-style della
libreria. ESP32 controlla Level, Tune, Attack, Decay, Sweep e Drive. Il percorso
dry resta a livello pieno: `Drive 0%` e' pulito, mentre valori superiori
miscelano progressivamente la saturazione senza essere necessari per rendere
udibile la voce.
Il controllo `Utility > Output Volume` della ESP32 viene applicato alla fine
della catena stereo e regola insieme voci, kick, effetti e test audio.

Il synth interno e' ora organizzato come quattro `Voice` indipendenti. Ogni
voce contiene tre oscillatori, noise, filtro multimode, VCA, tre ADSR, due LFO
e una modulation matrix fixed-size. UART v5 riceve due banchi indipendenti di
tre oscillatori. V1 e V2 riservano le prime due voci DSP durante il sequencer
o il preview; il kick resta separato. Le altre due voci restano disponibili
all'allocatore polifonico.
I preset sono gestiti dalla ESP32 nel formato v9 con migrazione da v3-v8.

Le 16 forme mostrate dalla UI sono tutte implementate nel DSP. Quando OSC1,
OSC2 e OSC3 sono abilitati e hanno livello maggiore di zero, ogni evento
melodico del sequencer li apre insieme nella voce della propria traccia.
Note, pause e modifiche ai tre oscillatori di V1 non alterano quelli di V2.

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
