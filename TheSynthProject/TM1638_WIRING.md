# LED&KEY TM1638N Step Sequencer

STORICO: questo cablaggio non e piu usato dal firmware attuale. Il sequencer
hardware e stato sostituito da `4x Adafruit NeoKey 1x4 QT` su STEMMA QT.
Usa invece `NEOKEY_STEMMAQT_WIRING.md` e `PINOUT_ESP32_NEOKEY.md`.

Per il pinout della tua ESP32 DevKit V1 fotografata, usa prima
`PINOUT_ESP32_TM1638.md`.

Collega il modulo storico al progetto `TheSynthProject` cosi:

| TM1638N | ESP32 |
| --- | --- |
| VCC | 5V |
| GND | GND |
| STB / STROBE | D21 |
| CLK | D19 |
| DIO | TX2 |

La tua scheda LED&KEY e venduta come modulo 5V, quindi la configurazione
corretta e VCC a 5V. VCC a 3V3 e solo una prova sicura per non rischiare i pin
ESP32: alcuni moduli a 5V, a 3V3, restano deboli o non funzionano in modo
affidabile.

Se alimenti il modulo a 5V, non collegare `DIO`, `CLK` e `STB` direttamente
all'ESP32. Usa un level shifter 3V3/5V:

| Segnale | ESP32 | Level shifter | TM1638 |
| --- | --- | --- | --- |
| 3V3 | 3V3 | LV | - |
| 5V | - | HV | VCC |
| GND | GND | GND | GND |
| STB | D21 | LV1/HV1 | STB |
| CLK | D19 | LV2/HV2 | CLK |
| DIO | TX2 | LV3/HV3 bidirezionale | DIO |

`DIO` e bidirezionale: e il pin piu pericoloso da collegare diretto a 5V,
perche il modulo puo riportare 5V verso il pin ESP32.
Tieni i fili corti e metti, se puoi, un condensatore da 100 nF piu uno da
10 uF tra VCC e GND vicino al modulo.

## Con i componenti che hai: SN74AHCT125N

Tra i moduli/componenti disponibili, usa lo `SN74AHCT125N`. Alimentalo a 5V:

| SN74AHCT125N | Collegamento |
| --- | --- |
| VCC | 5V |
| GND | GND comune |
| /OE canale STB | GND |
| A canale STB | ESP32 D21 |
| Y canale STB | TM1638 STB |
| /OE canale CLK | GND |
| A canale CLK | ESP32 D19 |
| Y canale CLK | TM1638 CLK |
| /OE canale DIO | ESP32 RX2 |
| A canale DIO | ESP32 TX2 |
| Y canale DIO | TM1638 DIO |

Per leggere `DIO` dal TM1638 verso ESP32 serve anche un partitore:

```text
TM1638 DIO -- 10k --+-- ESP32 TX2
                    |
                   20k
                    |
                   GND
```

Il firmware usa `RX2` per spegnere il buffer DIO mentre legge i tasti:
`/OE` e attivo basso, quindi `LOW` abilita e `HIGH` disabilita.

Esistono schede TM1638 molto simili ma non identiche:

| Sigla comune | LED | Tasti | Note |
| --- | --- | --- | --- |
| TM1638 LED & KEY | 8 rossi | 8 | Modulo classico con serigrafia LED&KEY |
| TM1638 V1.3 / LKM1638 | 8 bicolore | 8 | Alcuni cloni usano bit LED diversi |
| TM1638 KEYS / QYF | 0 | 16 | Non e la scheda giusta per questa UI |

Il codice scrive i LED direttamente con valore `0xFF`, cosi copre anche cloni
che non usano il solito bit LED del modulo classico. All'avvio mostra `LEDTEST`,
accende tutti gli 8 LED e poi fa una scansione `LED 1` ... `LED 8`: se un LED
non si accende in questa scansione, il problema e hardware/alimentazione/mappa
elettrica, non il sequencer BPM.

Dopo `LEDTEST` parte anche una scansione grezza `ADR C0` ... `ADR CF`: il
firmware scrive `0xFF` su ogni indirizzo RAM del TM1638. Se i LED mancanti si
accendono durante uno di questi indirizzi, annota l'indirizzo mostrato: basta
rimappare quegli indirizzi nel codice. Se non si accendono mai, il problema e
quasi certamente fisico o di alimentazione.

## Libreria

Installa la libreria `TM1638plus` dall'Arduino Library Manager cercando
`tm1638`. Il codice usa:

```cpp
#include <TM1638plus.h>
```

Su ESP32 il codice usa la modalita high-frequency della libreria e rallenta la
lettura/scrittura DIO. Il default e prudente (`20 us`) per funzionare anche con
level shifter economici.

Il tutorial di aspettandoilbus usa invece la libreria `TM1638lite`. Il modulo e
lo stesso tipo classico `TM1638 LED&KEY`, ma la libreria non e identica. Per
escludere al 100% un problema di libreria, prova il demo originale
`File > Examples > TM1638lite > demo` su Arduino Nano a 5V, senza ESP32 e senza
level shifter. Se anche quel demo non accende tutti gli 8 LED, il problema e il
modulo, la breadboard, i cavi o le saldature. Se invece funziona, si puo
portare il firmware da `TM1638plus` a `TM1638lite`.

## Mappa tasti

| Tasto | Azione |
| --- | --- |
| S1 | Step 1 ON / REST sulla track attiva |
| S2 | Step 2 ON / REST sulla track attiva |
| S3 | Step 3 ON / REST sulla track attiva |
| S4 | Step 4 ON / REST sulla track attiva |
| S5 | Step 5 ON / REST sulla track attiva |
| S6 | Step 6 ON / REST sulla track attiva |
| S7 | Step 7 ON / REST sulla track attiva |
| S8 | Step 8 ON / REST sulla track attiva |

Ogni tasto seleziona anche lo step corrispondente, cosi lo schermo LCD e
l'encoder possono modificarne voce, nota, shape, BPM, length, direction, gate,
transpose e utility dal menu sequencer.

Quando il sequencer e fermo, gli 8 LED mostrano il pattern della track attiva,
V1 o V2:

| LED | Stato |
| --- | --- |
| acceso | step attivo |
| spento | rest |

Il pattern parte vuoto su entrambe le track: tutti gli step sono `REST`, quindi
premendo un tasto aggiungi quello step alla voce selezionata. Premendolo di
nuovo lo rimetti in `REST`.

Il display del modulo mostra voce e step selezionati, per esempio `V1S3+05` o
`V2S3RST`.

Quando il transport del sequencer e in RUN, il display LED&KEY mostra il tempo,
per esempio `120 BPM`, e i LED passano in chase mode: un solo LED segue lo step
corrente del clock audio, in stile TR-909. Da fermo torna la vista pattern/edit.

Non ci sono comandi utility, play/stop, BPM, selezione voce o menu sui tasti
LED&KEY: quelli restano sullo schermo LCD con encoder.
