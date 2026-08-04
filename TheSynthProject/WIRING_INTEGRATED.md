# Cablaggio integrato: un ESP32, TFT + 4 OLED + 5 encoder

Questa è la pinmap attiva di `TheSynthProject`. Sostituisce il vecchio pinout NeoKey e
quello del `synthProject` autonomo.

> **Backend audio:** la Daisy è l'unica uscita audio. Per il collegamento
> ESP32-Daisy e per i pin audio 18/19/20 seguire
> [`DAISY_MIGRATION.md`](DAISY_MIGRATION.md).

## Regola obbligatoria per i nomi dei pin ESP32

In questo progetto i pin della ESP32 vengono indicati **solo con la serigrafia
stampata sulla board della foto**: per esempio `D23`, `D14`, `RX2`, `TX2`,
`3V3` e `GND`. Non usare nomi numerici alternativi.

La disposizione completa dei nomi è in
[`ESP32_PIN_NAMES.md`](ESP32_PIN_NAMES.md).

I nomi e l'ordine fisico delle serigrafie di espansore, PCA9548A, TFT, OLED ed
encoder sono definiti in
[`COMPONENT_PIN_NAMES.md`](COMPONENT_PIN_NAMES.md). Le tabelle seguenti vanno
lette usando quella legenda, senza dedurre collegamenti dai fili delle foto.

## Hardware necessario

- 1x ESP32 DevKit V1 con ESP32-WROOM-32;
- 1x TFT ST7735 160x128 SPI già usato da `TheSynthProject`;
- 1x encoder master con pulsante e 1x pulsante back;
- 1x PCA9548A, indirizzo `0x70`;
- 3x OLED AZDelivery 1,3 pollici SH1106 128x64 I2C, indirizzo `0x3C`;
- 1x OLED piccolo SSD1306 128x64 I2C, indirizzo `0x3C`;
- 4x encoder rotativi con pulsante;
- 1x MCP23017 a 3,3 V, indirizzo `0x20`;
- 4x Adafruit NeoKey 1x4 QT, indirizzi `0x30`-`0x33`;
- 1x Daisy Seed per audio, sequencer e DSP;
- massa comune, cablaggi corti e rail 5 V / 3,3 V adeguati.

Il MCP23017 è l'unico componente aggiunto rispetto ai due setup originali. È
necessario per mantenere davvero cinque encoder, il tasto back e tutte le altre
periferiche senza assegnare i dodici nuovi segnali encoder ad altri pin di
boot o alla seriale USB.

## Partire dalla board 4 encoder/4 OLED già montata

Usare quella board come base. Questi collegamenti non cambiano:

- PCA9548A e quattro OLED, compresi i canali 0-3;
- alimentazione 3V3 e massa comune di PCA/OLED;
- posizione fisica dei quattro encoder;
- alimentazione `+` a 3V3 e `GND` di ogni encoder.

Prima della modifica è consigliato caricare ancora una volta lo sketch in
`../examples-and-tests/legacy-four-oled/` e verificare che tutti i componenti
funzionino. Poi spegnere e scollegare USB/alimentazione.

### Fili encoder da spostare

Scollegare ciascun filo segnale dal vecchio pin ESP32 e collegare lo stesso
filo al pin MCP23017 indicato. Non lasciare il vecchio pin ESP32 collegato in
parallelo.

| Segnale già montato | Vecchia serigrafia ESP32 | Nuova serigrafia espansore |
| --- | --- | --- |
| E1 CLK | D23 | A0 della riga `B0/A0` |
| E1 DT | D14 | A1 della riga `B1/A1` |
| E1 SW | D13 | A2 della riga `B2/A2` |
| E2 CLK | D27 | A3 della riga `B3/A3` |
| E2 DT | D26 | A4 della riga `B4/A4` |
| E2 SW | D25 | A5 della riga `B5/A5` |
| E3 CLK | D33 | A6 della riga `B6/A6` |
| E3 DT | D32 | A7 della riga `B7/A7` |
| E3 SW | D18 | B0 della riga `B0/A0` |
| E4 CLK | D19 | B1 della riga `B1/A1` |
| E4 DT | RX2 | B2 della riga `B2/A2` |
| E4 SW | TX2 | B3 della riga `B3/A3` |

### Nota importante: piedinatura reale di E1

Gli encoder montati hanno i terminali `CLK`, `DT`, `SW`, `+` e `GND`. Per encoder 1
il cablaggio reale comunicato è `D23 - D14 - D13` nell'ordine
`CLK - DT - SW`:

| Terminale E1 | Filo esistente | Nuova destinazione |
| --- | --- | --- |
| CLK | D23 | A0 della riga `B0/A0` |
| DT | D14 | A1 della riga `B1/A1` |
| SW | D13 | A2 della riga `B2/A2` |
| + | 3V3 | 3V3, invariato |
| GND | GND | GND comune, invariato |

Etichettare quindi i tre fili prima di scollegarli e trasferirli singolarmente.
Seguire le scritte `CLK`, `DT`, `SW`, non la precedente tabella ricavata dal
vecchio commento software.

Se la board è su millefori, basta dissaldare/tagliare le dodici piste sul lato
ESP32 e aggiungere una piccola daughterboard MCP23017. Non modificare i fili GND
degli encoder.

La pinmap risultante è:

- TFT: D5, D4, D2, D18, D23;
- encoder master/back: D32, D33, D25, D26;
- D27, D14 e D13 liberi;
- bus MCP23017 indipendente: D19 e D15.

Infine aggiungere la catena dei quattro NeoKey al canale 4 del PCA9548A. In pratica
la vecchia control surface diventa la base meccanica/elettrica e `TheSynthProject`
aggiunge TFT, encoder master, back e il software completo.

## Tabella completa ESP32

| Serigrafia ESP32 | Collegamento | Direzione |
| --- | --- | --- |
| D2 | TFT RST | uscita |
| D4 | TFT DC/A0 | uscita |
| D5 | TFT CS | uscita |
| D13 | libero | — |
| D14 | libero | — |
| D15 | MCP23017 SCL, bus I2C indipendente | uscita open-drain |
| D18 | TFT SCK/CLK | uscita SPI |
| D19 | MCP23017 SDA, bus I2C indipendente | bidirezionale |
| D21 | SDA principale del PCA | bidirezionale |
| D22 | SCL principale del PCA | uscita open-drain |
| D23 | TFT SDA/MOSI | uscita SPI |
| D25 | pulsante encoder master | ingresso pull-up |
| D26 | pulsante back K0 | ingresso pull-up |
| D27 | libero | — |
| D32 | encoder master A/CLK | ingresso pull-up |
| D33 | encoder master B/DT | ingresso pull-up |
| RX2 | Daisy D13 / pin fisico 14 | ingresso UART, backend Daisy |
| TX2 | Daisy D14 / pin fisico 15 | uscita UART, backend Daisy |

D34, D35, VP e VN restano liberi e sono solo input. TX0, RX0 e D12 non vengono
usati. Il TFT conserva D2 e D5,
che sono pin di strap ma facevano già parte del cablaggio `TheSynthProject` funzionante:
non aggiungere pull-up/pull-down esterni su questi segnali.

## 1. Modulo TFT, nello stesso ordine fisico della foto 3

Con display frontale e fila pin a sinistra, leggere dal basso verso l'alto:

| TFT | ESP32 |
| --- | --- |
| GND | GND comune |
| VCC | 3V3 se il modulo accetta 3,3 V, come nel setup `TheSynthProject` già funzionante |
| SCL | D18 |
| SDA | D23 |
| RES | D2 |
| DC | D4 |
| CS | D5 |
| BLK | 3V3; aggiungere la resistenza prevista dal proprio modulo se non è integrata |
| A | D32 |
| B | D33 |
| PUSH | D25 |
| K0 | D26 |

Il firmware non usa MISO. Non cambiare l'alimentazione di un TFT già
funzionante senza controllare la serigrafia/datasheet della sua breakout.

## 2. Encoder master e back

Sul modulo TFT i nomi da usare sono, in ordine, `A`, `B`, `PUSH`, `K0`, come
riportato nella tabella precedente. I contatti comuni dell'encoder e del tasto
back vanno a GND comune.

Gli ingressi usano `INPUT_PULLUP`: premuto equivale a livello LOW.

## 3. Bus I2C principale: PCA9548A + NeoKey

| Bus principale | ESP32 |
| --- | --- |
| SDA | D21 |
| SCL | D22 |
| logica/alimentazione | 3V3 |
| massa | GND comune |

### PCA9548A

| PCA9548A | Collegamento |
| --- | --- |
| VIN | 3V3 |
| GND | GND comune |
| SDA | D21 |
| SCL | D22 |
| A0, A1, A2 | GND/default, per indirizzo `0x70` |
| RST | pull-up a 3V3; sulla breakout è normalmente già presente |

OLED:

| OLED | Canale PCA9548A |
| --- | --- |
| OLED sopra E1, AZDelivery SH1106 | SD0 + SC0 |
| OLED sopra E2, AZDelivery SH1106 | SD1 + SC1 |
| OLED sopra E3, AZDelivery SH1106 | SD2 + SC2 |
| OLED sopra E4, piccolo SSD1306 | SD3 + SC3 |
| catena 4x NeoKey | SD4 + SC4 |

Controllare sempre la serigrafia del singolo modulo prima di collegarlo; sui
moduli attuali l'ordine e' `GND`, `VCC`, `SCL`, `SDA`. VCC a 3V3, GND comune,
indirizzo `0x3C`. Non collegare gli OLED
direttamente a D21/D22: SDA/SCL devono andare al rispettivo canale del mux.
Ogni canale I2C deve avere pull-up: le normali breakout OLED li includono;
con display nudi aggiungere 4,7 kOhm da SDx e SCx a 3V3 su ciascun canale usato.

Il firmware usa automaticamente il driver SH1106 sui canali 0-2 e SSD1306 sul
canale 3. La mappa e' configurata da `SURFACE_OLED_SH1106_MASK` in
`HardwarePins.h`; il valore previsto per questa configurazione e' `0x07`.

### NeoKey

| NeoKey | Indirizzo | Collegamento |
| --- | --- | --- |
| step 1-4 | `0x30` | SDA→SD4, SCL→SC4, 3V3, GND |
| step 5-8 | `0x31` | concatenato via STEMMA QT al primo; chiudere solo jumper A0 |
| step 9-12 | `0x32` | concatenato; chiudere solo jumper A1 |
| step 13-16 | `0x33` | concatenato; chiudere jumper A0 e A1 |

La catena entra nel primo NeoKey tramite SD4/SC4 e prosegue attraverso le prese
STEMMA QT libere fino al quarto NeoKey. Non usare SD5/SC5. Le board NeoKey
includono pull-up I2C; con cavi corti non aggiungere altri pull-up se non
necessari.

L'ordine degli step segue il montaggio fisico da sinistra verso destra: prima i
moduli `0x30`, `0x31`, `0x32`, `0x33`, poi i quattro tasti di ogni modulo nello
stesso verso.

## 4. MCP23017 e quattro encoder

Il MCP usa un secondo controller I2C indipendente su D19/D15. PCA e relativi
canali OLED/NeoKey usano il bus principale D21/D22.

| Serigrafia espansore | Collegamento |
| --- | --- |
| VCC, lato controllo | 3V3 |
| GND, lato controllo | GND comune |
| SDA/SI | D19 |
| SCL/SCK | D15 |
| A0, A1, A2 del lato controllo | GND, indirizzo `0x20` |
| RESET | 3V3 tramite 10 kOhm; non lasciarlo flottante |
| ITA, ITB della riga `ITB/ITA` | non collegati |

Mettere un pull-up da 4,7 kOhm tra SDA e 3V3 e un altro da 4,7 kOhm tra SCL e
3V3 se la breakout MCP23017 non li contiene.

Mappa encoder:

| Encoder | CLK | DT | SW | + | GND |
| --- | --- | --- | --- | --- | --- |
| E1 | A0 della riga `B0/A0` | A1 della riga `B1/A1` | A2 della riga `B2/A2` | 3V3 | GND |
| E2 | A3 della riga `B3/A3` | A4 della riga `B4/A4` | A5 della riga `B5/A5` | 3V3 | GND |
| E3 | A6 della riga `B6/A6` | A7 della riga `B7/A7` | B0 della riga `B0/A0` | 3V3 | GND |
| E4 | B1 della riga `B1/A1` | B2 della riga `B2/A2` | B3 della riga `B3/A3` | 3V3 | GND |

B4, B5, B6 e B7 restano liberi. Il firmware abilita i pull-up interni dell'espansore,
quindi non servono resistenze aggiuntive sui singoli `CLK`, `DT` e `SW`.

## Alimentazione

- Tutte le masse devono essere collegate.
- I bus I2C e il MCP23017 devono avere pull-up a 3,3 V, mai a 5 V.
- Per il primo collaudo alimentare la ESP32 da USB e tenere basso il volume.
- Se compaiono reset, OLED instabili o rumore, usare una 3,3 V regolata esterna
  adeguata per display/periferiche, con massa comune. Non unire alla cieca due
  uscite regolatore 3,3 V e non alimentare il pin 3V3 da due sorgenti.

Il consumo esatto dipende dalle breakout e dalla retroilluminazione del TFT;
per questo non è corretto promettere che ogni DevKit possa alimentare tutto dal
proprio regolatore senza conoscere i moduli specifici.

## Sequenza di montaggio e collaudo

1. Spegnere tutto e fare prima le linee GND e 3V3.
2. In Arduino IDE scegliere `ESP32 Dev Module`; la partizione standard e' sufficiente.
3. Collegare TFT, encoder master e back. Caricare `TheSynthProject.ino` e verificare il TFT.
4. Collegare PCA9548A e un OLED per volta sui canali 0, 1, 2, 3.
5. Collegare la catena NeoKey a SD4/SC4 e verificarla in
   `Utility > OLED Debug`: devono risultare presenti `0x30`-`0x33`.
6. Collegare MCP23017 senza encoder e verificarlo in `Utility > MCP Debug`.
7. Collegare E1, testarlo in `ENCODER VIEW`, poi ripetere E2, E3 ed E4.
8. Collegare UART e uscita audio Daisy come in `DAISY_MIGRATION.md`.
9. Aprire `DIAGNOSTICS`: il mask OLED corretto è `0xF`, PCA e MCP devono essere `OK`.
10. Provare `SYNTH CONTROL`, il kick e infine `PONG`; `WAVEFORM` è solo visuale.

Serial Monitor: `115200 baud`.

La diagnostica seriale parte attiva e stampa la checklist delle connessioni
durante il boot; sul TFT resta lo splash grafico originale. Il riepilogo puo'
essere aperto anche con `Utility > Connection Status`; ruotare l'encoder master
per ripetere le scansioni. `Utility > Serial Diagnostics` apre la pagina senza
cambiare lo stato; ruotare o premere per attivare/disattivare i report seriali
periodici, inclusa la telemetria bidirezionale ESP32-Daisy.

## Debug OLED dal TFT principale

Aprire `Utility > OLED Debug`. La pagina funziona anche con tutti i
quattro OLED spenti. Entrando viene eseguita una scansione; ruotare l'encoder
master per ripeterla dopo avere corretto un filo.

Valori attesi:

- `D21:H` e `D22:H`: entrambe le linee sono alte quando il bus è a riposo;
- `PCA:70`: PCA9548A rilevato all'indirizzo `0x70`;
- `ACK:F`: un OLED risponde su tutti e quattro i canali;
- `INIT:F`: tutti e quattro gli OLED sono stati inizializzati;
- `1:3C 2:3C 3:3C 4:3C`: indirizzo OLED per ciascun canale; il rapporto seriale
  indica anche `SH1106` per i primi tre e `SSD1306` per il quarto;
- `K 30+31+32+33+`: tutti e quattro i NeoKey sono presenti sul canale PCA 4.

Interpretazione rapida:

- `D21:L` o `D22:L`: probabile corto, inversione o dispositivo che blocca il bus;
- `PCA:--`: controllare `VIN`, `GND`, `SDA`, `SCL`, `RST` e `A0/A1/A2` del PCA;
- `ACK:0`: PCA presente ma nessun OLED risponde; controllare alimentazione e
  ordine `GND`, `VCC`, `SCL`, `SDA` degli OLED;
- un solo bit mancante in `ACK`: controllare il relativo canale `SCx/SDx`;
- bit presente in `ACK` ma assente in `INIT`: risposta elettrica presente ma
  inizializzazione del controller OLED configurato fallita.

Il Serial Monitor a `115200` mostra anche tutti gli indirizzi del bus principale
e una riga distinta per ciascun canale PCA.

## Debug MCP23017 dal TFT principale

Aprire `Utility > MCP Debug`. Entrando viene scansionato il bus indipendente
degli encoder; ruotare l'encoder principale del TFT per ripetere il test dopo
avere corretto un collegamento. Il test puo' inizializzare un MCP collegato dopo
il boot senza riavviare la ESP32.

Valori attesi con MCP collegato ma encoder scollegati:

- `D19:H` e `D15:H`;
- `BUS:1` e `ADDR:20`;
- `RAW:FFFF`, perche' i sedici ingressi hanno i pull-up attivi;
- contatori `E1`, `E2`, `E3`, `E4` inizialmente a zero.

Collegando un encoder alla volta, il rispettivo contatore deve cambiare durante
la rotazione. Nel rapporto seriale a 115200 l'ordine di ogni terna e'
`CLK`, `DT`, `SW`: a riposo sono `HIGH`; premendo la manopola `SW` diventa
`LOW`. Se `D19` o `D15` sono `LOW`, spegnere e cercare un corto. Se entrambe
sono `HIGH` ma `BUS:0`, controllare alimentazione, `RESET`, indirizzo e che il
chip sia realmente marcato `MCP23017`, non `MCP23S17`.

## Comandi rapidi

| Modalità | Comandi |
| --- | --- |
| Synth Control | gira E1-E4; push E1 pagina indietro, E2 avanti, E3 azione, E4 menu |
| Encoder View | gira tutti; push E1-E3 azzera; push E4 menu |
| Waveform | E1 parametro A/forma precedente, E2 parametro B/forma successiva, E4 menu |
| Pong | gira E1 paddle sinistro, E4 paddle destro; a game over push E1/E4 reset |
| Diagnostics | osserva bus/contatori; E4 menu |
| Diagnostica seriale | `Utility > Serial Diagnostics`; ruota o premi per attivare/disattivare |
| Stato connessioni | `Utility > Connection Status`; ruota per ripetere le scansioni |

## Riferimenti hardware

- ESP32 Arduino I2C: <https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html>
- ESP32 dispone di due controller I2C: <https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32/api-reference/peripherals/i2c.html>
- MCP23017 datasheet: <https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf>
- PCA9548A datasheet: <https://www.nxp.com/docs/en/data-sheet/PCA9548A.pdf>
- NeoKey pinout e indirizzi: <https://learn.adafruit.com/neokey-1x4-qt-i2c/pinouts>

## Debug Daisy

Aprire `Utility > Connection Status` oppure il Monitor Seriale a 115200. Devono
risultare `ESP > DAISY: ACK` e `DAISY > ESP: RX`. Dopo un riavvio della ESP32 la
Daisy riattiva automaticamente il proprio ricevitore; se `WAIT ACK` persiste,
controllare TX2 verso D14/pin 15, la massa digitale comune e che entrambi i
firmware usino la stessa versione del protocollo.
