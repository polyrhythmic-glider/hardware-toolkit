# Formato preset

I preset persistenti appartengono alla ESP32 e sono salvati in `Preferences`,
namespace `polyglider`. Il formato corrente è **v9**.

## Versioni e migrazioni

| Versione | Contenuto storico | Migrazione corrente |
| --- | --- | --- |
| v3 | 8 step con offset di nota | converte in pitch MIDI assoluti e inizializza il nuovo patch |
| v4 | 16 step, senza modulation matrix | crea `ENV1 -> Amp`, inizializza ENV2/3, LFO e OSC3 |
| v5 | 16 step con ADSR/route, note come offset | converte gli offset, poi passa attraverso la migrazione v7 |
| v6 | 16 step con pitch MIDI assoluti | copia i primi 16 step e inizializza 17-64 come rest |
| v7 | 64 step, pagina, auto-scroll, due ENV e route legacy | traduce le route legacy nella matrice moderna e applica default neutri ai campi nuovi |
| v8 | patch moderno completo più sequencer a 64 step | separa il banco condiviso nei banchi V1 e V2 |
| v9 | due banchi di tre oscillatori e livelli indipendenti V1/V2 | caricamento diretto |

Il loader distingue prima la dimensione del record, poi valida magic e
versione. I layout legacy sono struct separate: l'espansione della matrice v8
non cambia la dimensione attesa dei preset v3-v7. Il record v9 conserva tutto
il prefisso v8 e aggiunge il banco V2 completo e il livello master di V1.

La migrazione del banco condiviso mantiene OSC1 come primo oscillatore di V1
e copia OSC2 nel primo oscillatore di V2. OSC2 viene disabilitato in entrambi
i nuovi banchi; l'eventuale OSC3 viene copiato in entrambi. I livelli master
di V1 e V2 partono al 100%, mantenendo i livelli dei singoli oscillatori.

Durante la migrazione v7, le destinazioni ancora valide vengono preservate.
Le vecchie route aggregate vengono tradotte alla destinazione moderna più
vicina (`Oscillator Mix`/`FM` verso `All Osc Pitch`, `PWM` verso OSC1 PWM,
`FX Mix`/`Delay Feedback` verso Delay Mix). Le vecchie destinazioni che il DSP
non applicava vengono ignorate.

## Campi aggiunti in v8

- parametri completi e flag enabled di OSC1, OSC2 e OSC3;
- ENV1, ENV2 ed ENV3 con enabled, amount, velocity sensitivity e retrigger;
- LFO1 e LFO2 con waveform, frequenza, phase, polarità, amount, retrigger,
  free-running e placeholder tempo-sync;
- tabella di route per ENV1-3 e LFO1-2 sulle destinazioni moderne;
- filter resonance, noise level e pan della voce;
- impostazioni già presenti del sequencer: 64 step, pagina e auto-scroll.

La migrazione da v3-v7 inizializza i campi moderni con questi default:

- OSC3 disabilitato e level 0;
- ENV1 abilitato e assegnato ad Amp a `+100%`;
- ENV2 ed ENV3 disabilitati e senza route;
- LFO1 e LFO2 disabilitati;
- nuove route inattive;
- quattro voci disponibili nel motore Daisy.

Il record persistente conserva più route di quante ne vengano inviate in uno
snapshot UART. Il protocollo v5 trasporta al massimo 16 route abilitate, in
ordine di sorgente e destinazione, che è anche il limite fixed-size di ogni
voce Daisy.
