# Architettura DSP Daisy

Il motore audio della Daisy usa un'architettura polifonica a dimensione fissa.
Le costanti pubbliche in `SynthEngine.h` sono:

```cpp
constexpr size_t kMaxVoices = 4;
constexpr size_t kSequencerVoices = 2;
constexpr size_t kOscillatorsPerVoice = 3;
constexpr size_t kEnvelopesPerVoice = 3;
constexpr size_t kLfosPerVoice = 2;
constexpr size_t kMaxModRoutesPerVoice = 16;
```

Non sono usati contenitori a crescita dinamica. `SynthEngine` possiede
direttamente un `std::array<Voice, 4>` e ogni `Voice` possiede direttamente i
propri processor e le proprie route.

## Flusso del segnale

Il percorso di ogni voce è:

```text
OSC1 -----\
OSC2 ------+--> mixer --> filtro multimode --> VCA --> pan --> uscita voce L/R
OSC3 ------+
Noise ----/
```

OSC1, OSC2, OSC3 e noise hanno livelli indipendenti prima del mixer. Ogni
oscillatore conserva `enabled`, waveform, level, octave, semitone, fine tune,
phase offset, pulse width, wavefold, pan, key tracking e modulation input
amount. Le forme 0-5 (`SINE`, `SQUARE`, `TRI`, `SAW`, `RAMP`, `PULSE`) e
`PWM` usano gli oscillatori DaisySP; il wrapper fixed-size implementa inoltre
`SUB`, `ORGAN`, `EVEN`, `ODD`, `SUPER`, `STAIR`, `CLIP`, `NOISE` e `FORMNT`.
Tutti i 16 nomi esposti dalla UI producono quindi una forma audio definita e
nessuno ricade silenziosamente su Sine.

Il filtro `Svf` è per-voice e seleziona Low Pass, High Pass o Band Pass. Il VCA
riceve la somma della destinazione `Amp` della modulation matrix più il valore
base, quindi non è legato direttamente a un ADSR specifico.

Il percorso globale resta:

```text
Voice 1 --\
Voice 2 ---+
Voice 3 ---+--> mix polifonico --\
Voice 4 --/                       +--> delay --> reverb --> master --> L/R
Kick 909 ------------------------/
```

Il test a 440 Hz conserva il bypass diretto. Kick 909, delay, reverb, volume
master, link fade e clamp stereo restano processor globali.

## Voice e allocazione

Ogni `Voice` contiene tre `VoiceOscillator`, tre `Wavefolder`, un noise
generator a stato `uint32_t`, un
mixer, un `Svf`, un VCA, tre `Adsr`, due LFO, una `ModulationMatrix`, flag
active/gate, nota MIDI, velocity, allocation age e gli smoother necessari.
Nessuno stato di envelope, LFO o filtro è condiviso tra voci.

`SynthEngine::NoteOn()` seleziona il primo slot inattivo. Se tutti gli slot sono
occupati, sostituisce quello con allocation age minore, cioè la voce attiva più
vecchia. `NoteOff(note)` rilascia le voci con quella nota e `AllNotesOff()`
chiude tutti i gate lasciando completare le release.

`ReserveSequencerVoices()` riserva gli slot 0 e 1 per V1 e V2 durante il
sequencer o il preview. `TriggerSequencerVoice()` e `SetSequencerGate()`
gestiscono note e gate separati. Ogni traccia possiede tre oscillatori:
un evento apre quelli abilitati del proprio banco, senza alterare l'altra
traccia. UART v5 trasporta i due banchi e seleziona separatamente voce e
oscillatore del preview. `NoteOn` e `NoteOff` escludono entrambi gli slot
riservati; le altre due voci restano disponibili all'allocatore. Lo stop
rilascia entrambe le voci. L'API compatibility a voce singola resta disponibile
per i client precedenti del motore.

## Inviluppi e LFO

ENV1, ENV2 ed ENV3 espongono enabled, attack, decay, sustain, release, amount,
velocity sensitivity e retrigger Legato/Soft/Hard. Un inviluppo può alimentare
più route e più sorgenti possono sommarsi sulla stessa destinazione. Il default
compatibile abilita soltanto ENV1 e la route `Env1 -> Amp` a `+1`; ENV2 ed ENV3
non hanno destinazioni permanenti.

LFO1 e LFO2 sono indipendenti e supportano Sine, Triangle, Saw, Ramp, Square e
Sample and Hold. Espongono enabled, frequency, phase, bipolar/unipolar, amount,
retrigger on NoteOn e free-running. I campi tempo sync e division sono presenti
nel modello ma sono placeholder: finché non viene fornito un clock musicale al
motore, la frequenza resta espressa in Hz.

## Modulation matrix

Ogni voce ha 16 route fisse. Una route contiene enabled, source, destination,
amount firmato, curve placeholder e scope per-voice/global. Le sorgenti sono:

- `Env1`, `Env2`, `Env3`;
- `Lfo1`, `Lfo2`;
- `Velocity`, `NoteNumber`, `KeyTrack`.

Le destinazioni sono:

- `Amp`, `FilterCutoff`, `FilterResonance`;
- pitch OSC1/2/3 e `AllOscPitch`;
- level OSC1/2/3 e `NoiseLevel`;
- pulse width OSC1/2/3;
- wavefold OSC1/2/3;
- `Pan`, `DelayMix`, `ReverbMix`.

La matrix azzera le contribuzioni a ogni sample, somma tutte le route abilitate
e lascia intatti i valori base. Pitch, cutoff e parametri normalizzati vengono
scalati e clampati soltanto quando si calcola il valore DSP finale. Delay e
reverb sono destinazioni globali: `Voice` produce la contribuzione, mentre
`SynthEngine` la normalizza sul voice mix e il livello globale la applica in
modo sicuro.

## Realtime safety

Nel callback non avvengono allocazioni, resize, I/O bloccante o logging. Tutte
le collezioni sono `std::array`; le variabili temporanee sono array a dimensione
fissa sullo stack. Il parsing UART continua fuori dal rendering per-sample e
uno snapshot pubblicato viene applicato soltanto al confine del blocco audio.
Pitch, pulse width, wavefold e coefficienti filtro sono aggiornati a control
rate; level, VCA e pan sono smussati per-sample.

## Validazione all'avvio

Prima di `StartAudio()` viene eseguito un test deterministico fuori dal
callback. Il test abilita 4 voci, 12 oscillatori, 12 ADSR, 8 LFO, quattro filtri,
route per-voice/global, kick, delay e reverb; verifica inoltre che una quinta
nota sostituisca la voce più vecchia. Al termine tutti i processor vengono
reinizializzati.

Il test e' abilitato per default e puo' essere escluso dalla build con:

```sh
make clean
make SYNTH_ENABLE_STARTUP_VALIDATION=0
```

I risultati sono leggibili dal debugger nei simboli:

- `dspStartupValidationPassed`;
- `dspWorstCaseElapsedUs` per 1.024 frame;
- `dspWorstCaseLoadPercent`, rapportato al sample rate reale;
- `dspValidationPeak`.

La misura è intenzionalmente esterna al callback e non stampa nulla. Il valore
CPU attendibile deve essere letto sulla Daisy fisica; la cross-compilazione non
può misurare il tempo del Cortex-M7.

Il core polifonico ha inoltre un test nativo eseguibile senza hardware:

```sh
make test-host
```

Il test rende quattro note reali con la configurazione 12 oscillatori, 12 ADSR,
8 LFO e quattro filtri, verifica tutte le 16 forme, output finito/non nullo,
contributi globali, release e oldest-voice stealing. Controlla esplicitamente
che `EVEN`, `ODD` e `FORMNT` non coincidano con il fallback Sine, oltre a note,
pause, retrigger e indipendenza audio dei tre oscillatori fra V1 e V2.

## Risorse della build

Con toolchain ARM `-O2` e startup validation abilitata, la build verificata usa:

| Regione | Uso | Capacità | Percentuale |
| --- | ---: | ---: | ---: |
| FLASH | 101.336 B | 128 KiB | 77,31% |
| SRAM | 416.416 B | 512 KiB | 79,43% |
| RAM_D2 | 16.896 B | 288 KiB | 5,73% |
| SDRAM | 115.212 B | 64 MiB | 0,17% |

Il simbolo `synthEngine` include direttamente tutte le quattro voci. La sezione
heap riservata dal linker e' 0 B; libDaisy/newlib possono comunque esporre
simboli generici `malloc/free`, che non sono chiamati dal percorso DSP.

## Esposizione e limiti attuali

- UART v5 trasporta due banchi OSC1-3, parametri comuni ENV1-3 e LFO1-2, e
  16 route; i preset ESP32 v9 serializzano entrambi i banchi e migrano v3-v8.
- Il sequencer conserva due corsie melodiche indipendenti più kick. Ogni
  evento apre gli OSC abilitati del proprio banco.
- Gli OLED espongono entrambi i banchi in pagine con tre sottopagine; il menu
  TFT mantiene l'editor completo degli oscillatori di V1 e le route ENV.
- Tempo sync degli LFO è un placeholder.
- Il pan usa una legge lineare; non è ancora equal-power.
- Non c'è ancora un ingresso MIDI fisico collegato alle API polifoniche.
- La misura CPU reale richiede hardware Daisy e lettura dei simboli di
  validazione; il test host verifica correttezza, non il carico Cortex-M7.
