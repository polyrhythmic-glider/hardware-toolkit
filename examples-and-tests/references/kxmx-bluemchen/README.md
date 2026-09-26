# kxmx_bluemchen — reference di design

> **Stato:** solo reference. Questa cartella non e' inclusa da `DaisySynth`,
> `TheSynthProject` o da alcun `Makefile` della repository.

## Provenienza

- repository: <https://github.com/recursinging/kxmx_bluemchen>;
- revisione esaminata: [`a18b647f`](https://github.com/recursinging/kxmx_bluemchen/commit/a18b647f82fbdd548093de74d6c2c9e0d0f83878), 21 febbraio 2022;
- licenza upstream: [MIT](https://github.com/recursinging/kxmx_bluemchen/blob/main/LICENSE), copyright 2020 recursinging;
- file studiati: `src/kxmx_bluemchen.{h,cpp}`, `examples/hardware_test.cpp`,
  `examples/kompressor.cpp`, `Makefile` e `.gitmodules`.

`minimal_reference.cpp` e' una riscrittura ridotta del pattern architetturale,
non una copia del pinout o del firmware Bluemchen.

## Pattern base estratto

La repository upstream separa due livelli:

1. `Bluemchen` e' una facade della board. Possiede `DaisySeed`, encoder, ADC,
   MIDI, display e SD; `Init()` chiama piccoli helper privati per configurare
   ogni periferica.
2. Il file applicativo possiede DSP e stato del programma. Il `main()` inizializza
   la board e i processori, poi avvia l'audio; il callback elabora i blocchi,
   mentre il loop infinito gestisce le attivita' non audio.

La facade inoltra anche le operazioni libDaisy che interessano l'applicazione:
sample rate, block size, callback rate, start/stop audio e start/stop ADC. Gli
ingressi analogici sono raccolti in un array indicizzato da un `enum`, cosi'
il codice applicativo usa nomi logici invece di numeri ADC sparsi.

Sequenza di avvio osservata:

1. configurare e inizializzare `DaisySeed`;
2. inizializzare encoder, display, MIDI, ADC/controlli, SD e DAC dalla facade;
3. avviare l'ADC;
4. inizializzare i mapping `Parameter` e il DSP usando il sample rate della board;
5. avviare la callback audio;
6. entrare nel loop foreground per UI, MIDI, SD e altre operazioni lente.

L'upstream espone anche setter per sample rate e block size, ma gli esempi
esaminati lasciano i default. In un adattamento con valori personalizzati vanno
impostati prima di inizializzare i controlli che dipendono dal callback rate.

## Esempio minimale conservato

[`minimal_reference.cpp`](minimal_reference.cpp) mantiene soltanto il nucleo
del design:

- una classe `MinimalBoard` come unico punto di inizializzazione hardware;
- un helper privato per la configurazione audio;
- un adattatore tipizzato per avviare il callback;
- inizializzazione DSP prevista prima di `StartAudio()`;
- callback realtime separato dal loop foreground.

L'esempio fa un semplice pass-through stereo. Non contiene pin, controlli o
periferiche Bluemchen e non include `SynthEngine`, quindi non puo' modificare il
comportamento del synth corrente.

## Pattern di build upstream

Il `Makefile` upstream sceglie un'applicazione tramite `TARGET` e compila due
sorgenti: la facade della board e `examples/${TARGET}.cpp`. `libDaisy` e
`DaisySP` sono submodule fissati a revisioni precise. Se il pattern verra'
adottato qui in futuro, e' preferibile mantenere la facade come sorgente stabile
e selezionare esplicitamente il solo entry point desiderato nel `Makefile`.

## Cosa riusare e cosa no

Da riusare eventualmente:

- una facade per isolare pinout e inizializzazione della board;
- helper piccoli per periferica;
- storage statico e percorsi audio senza allocazioni;
- confine netto tra inizializzazione, callback audio e loop foreground;
- nomi logici per controlli e periferiche.

Da non importare alla cieca:

- i pin Bluemchen, specifici di quella scheda Eurorack;
- le API libDaisy del 2022 senza confronto con la versione locale;
- display, stringhe, file SD o parsing MIDI nel callback audio;
- lo stesso aggiornamento dei controlli eseguito sia dal callback sia dal loop,
  perche' introduce ownership temporale poco chiara e possibili race;
- l'accesso pubblico indiscriminato a tutte le periferiche: un accessor mirato
  o metodi della facade rendono piu' espliciti i confini.

Per `DaisySynth`, un'eventuale adozione futura dovrebbe lasciare `SynthEngine`
responsabile solo del DSP, mantenere UART/telemetria nel foreground e applicare
gli snapshot di controllo ai confini dei blocchi. Questa cartella documenta
soltanto quella direzione: non la attiva.
