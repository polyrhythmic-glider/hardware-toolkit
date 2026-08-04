# Audio su Daisy Seed

Il sistema separa nettamente i compiti:

- **ESP32**: TFT, OLED, encoder, NeoKey, menu, preset e modifica dei pattern;
- **Daisy Seed**: clock audio, sequencer sample-accurate, oscillatori e DSP;
- **UART**: trasporta soltanto controlli digitali a 3,3 V;
- **pin 18/19 della Daisy**: unica uscita audio stereo.

Il vecchio PCM5102 e il motore I2S ESP32 sono stati rimossi dal progetto. La
Daisy è l'unica sorgente audio supportata.

## Rimuovere il vecchio PCM5102

La configurazione consigliata usa soltanto l'uscita della Daisy. Per togliere il
PCM in sicurezza:

1. Spegnere entrambe le schede e scollegare tutti i cavi USB.
2. Scollegare dal PCM `VIN`, `GND`, `BCK`, `LCK` e `DIN`.
3. Scollegare anche `L`, `R` e la massa audio del PCM dal jack o dal monitor.
4. Lasciare il jack collegato esclusivamente a Daisy pin 18, 19 e 20.
5. Lasciare liberi `D27`, `D14` e `D13` sulla ESP32.

Non lasciare le uscite audio PCM e Daisy in parallelo, neppure con il vecchio
modulo non alimentato. Il firmware non contiene più un fallback PCM.

## Collegamenti minimi

I numeri `14`, `15`, `18`, `19`, `20` e `40` nella colonna Daisy sono i
**numeri fisici del connettore Seed**. `D13` e `D14` sono invece i nomi GPIO
usati da libDaisy.

| ESP32 DevKit | Daisy Seed | Funzione |
| --- | --- | --- |
| `TX2` (GPIO17) | pin fisico **15**, `D14 / USART1 RX` | controlli ESP32 -> Daisy |
| `RX2` (GPIO16) | pin fisico **14**, `D13 / USART1 TX` | stato/playhead Daisy -> ESP32 |
| `GND` | pin fisico **40**, `DGND` | riferimento digitale comune |

RX e TX sono incrociati. I due collegamenti seriali sono logica 3,3 V e non
richiedono un level shifter. Non collegare fra loro le uscite `3V3` delle due
schede.

Per il primo test è più semplice alimentare ESP32 e Daisy con due cavi USB e
unire soltanto `DGND`, `TX` e `RX` come nella tabella. Spegnere tutto prima di
spostare i fili.

## Jack audio già collegato

La connessione indicata è corretta:

| Daisy Seed | Jack stereo |
| --- | --- |
| pin fisico **18**, `AUDIO OUT L` | Tip / Left |
| pin fisico **19**, `AUDIO OUT R` | Ring / Right |
| pin fisico **20**, `AGND` | contatto di massa audio |

Su un connettore **TRS** la massa audio è lo Sleeve. Su un connettore **TRRS**
può essere Ring2 o Sleeve secondo lo standard e la breakout: seguire le sigle
della scheda o verificare con il tester, senza dedurlo dal colore dei fili.

Usare inizialmente un ingresso line di monitor attivi, mixer o interfaccia,
con volume basso. Il pin 20 è il ritorno audio del jack; per la UART usare il
`DGND` al pin 40.

## Compilazione e caricamento

Il progetto Daisy è in `../DaisySynth` e usa le librerie già presenti nel tuo
checkout di `DaisyExamples`:

```sh
cd /home/maurice/Documenti/hardware-toolkit/DaisySynth
make
make program-dfu
```

Il `Makefile` trova per default:

```text
/home/maurice/Documenti/DaisyExamples/libDaisy
/home/maurice/Documenti/DaisyExamples/DaisySP
```

Se le librerie si trovano altrove:

```sh
make LIBDAISY_DIR=/percorso/libDaisy DAISYSP_DIR=/percorso/DaisySP
```

Poi compilare e caricare `TheSynthProject.ino` sulla ESP32. Non esiste più una
selezione del backend: ESP32 e Daisy fanno sempre parte dello stesso sistema.

Il protocollo UART è versionato: dopo modifiche a `DaisyLinkProtocol.h` bisogna
caricare sia la ESP32 sia la Daisy. Versioni diverse vengono rifiutate e il
Monitor Seriale mostra che la Daisy riceve ma scarta i dati.

## Primo collaudo

1. Caricare prima `TheSynthDaisy` sulla Seed.
2. Collegare UART e masse a schede spente.
3. Accendere Daisy ed ESP32.
4. Il LED della Seed deve diventare acceso fisso quando riceve pacchetti validi.
5. Aprire `Utility > Audio Test`: sui due canali deve uscire una sinusoide a
   440 Hz.
6. Avviare il sequencer e provare prima una sola voce, senza delay e riverbero.
7. Aggiungere kick, filtro ed effetti uno alla volta.

Se il LED resta spento, controllare per primi `TX2 -> D14/pin 15` e la massa
digitale comune. Il filo di ritorno `D13/pin 14 -> RX2` serve per playhead e
diagnostica, ma la generazione audio può già ricevere i controlli con il solo
filo ESP32 TX.

La Daisy riavvia autonomamente il listener UART quando la ESP32 scompare o il
ricevitore segnala un errore. Nel Monitor Seriale, `rx-restart/error` mostra i
due contatori: un riavvio occasionale dopo un power cycle è previsto; un valore
che cresce continuamente indica cablaggio instabile, massa assente oppure
baudrate/firmware non corrispondenti.

## Funzioni migrate in questa fase

- due oscillatori;
- forme Sine, Square, Triangle, Saw, Ramp e Pulse;
- level, tune, fine tune e wavefold per voce;
- ENV1 come inviluppo di ampiezza;
- filtro LP/HP/BP;
- sequencer fino a 64 step, in quattro pagine da 16, e kick;
- delay, riverbero e test diretto a 440 Hz;
- watchdog: la Daisy sfuma a silenzio se non riceve più l'ESP32;
- trigger kick one-shot inviati dall'interfaccia e recupero automatico UART.

Le forme 7-16 usano per ora Sine come fallback. ENV2, modulation matrix e la
modalità storica `WAVEFORM` non sono ancora processate dal DSP Daisy. La pagina
`WAVEFORM` resta quindi un visualizzatore senza uscita audio.
