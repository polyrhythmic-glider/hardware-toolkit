# Pinout ESP32 DevKit V1 per TM1638 LED&KEY

STORICO: questo pinout non e piu quello attivo. Il firmware attuale usa
`PINOUT_ESP32_NEOKEY.md` con 4x Adafruit NeoKey 1x4 QT su STEMMA QT.

Questo file usa i nomi stampati sulla tua scheda ESP32 nella foto.

## Pin ESP32 nella foto

L'orientamento canonico è quello della foto 5: USB in alto, antenna in basso e
componenti rivolti verso chi guarda. La lista completa è mantenuta in
[`ESP32_PIN_NAMES.md`](ESP32_PIN_NAMES.md).

### Lato sinistro, dall'alto verso il basso

`3V3`, `GND`, `D15`, `D2`, `D4`, `RX2`, `TX2`, `D5`, `D18`, `D19`, `D21`,
`RX0`, `TX0`, `D22`, `D23`.

### Lato destro, dall'alto verso il basso

`VIN`, `GND`, `D13`, `D12`, `D14`, `D27`, `D26`, `D25`, `D33`, `D32`,
`D35`, `D34`, `VN`, `VP`, `EN`.

## Pin usati dal codice adesso

Questi sono i pin definiti in `HardwarePins.h`:

| Funzione | Serigrafia ESP32 |
| --- | --- |
| TM1638 STB | D21 |
| TM1638 CLK | D19 |
| TM1638 DIO | TX2 |
| TM1638 DIO_OE | RX2 |

## Cablaggio con SN74AHCT125N

Il TM1638 va alimentato a 5V. Lo SN74AHCT125N va alimentato a 5V.

### Alimentazione

| Collegamento | Dove va |
| --- | --- |
| SN74 pin 14 VCC | 5V |
| SN74 pin 7 GND | GND comune |
| TM1638 VCC | 5V |
| TM1638 GND | GND comune |
| ESP32 GND | GND comune |

### STB

| Da | A |
| --- | --- |
| ESP32 D21 | SN74 pin 2 |
| SN74 pin 3 | TM1638 STB |
| SN74 pin 1 / 1OE | GND |

### CLK

| Da | A |
| --- | --- |
| ESP32 D19 | SN74 pin 5 |
| SN74 pin 6 | TM1638 CLK |
| SN74 pin 4 / 2OE | GND |

### DIO, scrittura ESP32 verso TM1638

| Da | A |
| --- | --- |
| ESP32 TX2 | SN74 pin 9 |
| SN74 pin 8 | TM1638 DIO |
| ESP32 RX2 | SN74 pin 10 / 3OE |

Attenzione: `3OE` e attivo basso. Il firmware mette RX2 basso quando scrive e alto quando legge.

### DIO, lettura TM1638 verso ESP32

Serve il partitore resistivo:

```text
TM1638 DIO / SN74 pin 8 -- 10k --+-- ESP32 TX2 / SN74 pin 9
                                 |
                                20k o 22k
                                 |
                                GND
```

### Canale 4 SN74 non usato

| Pin SN74 | Collegamento |
| --- | --- |
| pin 13 / 4OE | 5V |
| pin 12 / 4A | GND |
| pin 11 / 4Y | non collegato |

## Pin da NON usare per questo cablaggio

| Serigrafia | Motivo |
| --- | --- |
| D13 | Non usato ora; lascia scollegato per evitare confusione |
| RX0/TX0 | Usati dalla seriale USB |
| VP/VN/D34/D35 | Solo input |
| D12/D15 | Pin di boot/strap, meglio evitarli |

## Test se i LED cambiano da una scheda TM1638 all'altra

Se cambiando modulo TM1638 cambiano anche i LED che funzionano, il problema non
e il BPM: puo essere contatto breadboard, cavo, saldatura, oppure mappa LED
diversa.

All'avvio il firmware mostra:

1. `LEDTEST`: tutti i LED comandati insieme.
2. `LED 1` ... `LED 8`: test della mappa LED standard.
3. `ADR C0` ... `ADR CF`: test grezzo degli indirizzi RAM del TM1638.

Durante `ADR C0` ... `ADR CF`, segnati per ogni indirizzo quali LED si
accendono. Esempio:

```text
ADR C1 -> LED 1
ADR C3 -> LED 5
ADR C5 -> nessuno
```

Con quella tabella si puo correggere `TM1638_LED_POSITION_MAP` o la funzione
`tm1638WriteLed()`.
