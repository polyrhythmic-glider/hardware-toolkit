# Cablaggio 2x Adafruit NeoKey 1x4 QT

Questo sostituisce completamente il vecchio modulo `TM1638 LED&KEY`.

## Pin ESP32 usati

| Funzione | Serigrafia ESP32 | GPIO reale |
| --- | --- | --- |
| I2C SDA | D21 | GPIO21 |
| I2C SCL | D19 | GPIO19 |
| Alimentazione | 3V3 | 3.3V |
| Massa | GND | GND |

Non usare l'I2C default ESP32 su `GPIO22`, perche nel progetto `GPIO22` e gia
usato dall'audio `PCM5102_DIN`.

## Collegamento STEMMA QT / Qwiic

Se usi il cavo STEMMA QT a 4 fili:

| Filo STEMMA QT | Dove va su ESP32 |
| --- | --- |
| Rosso / VIN | 3V3 |
| Nero / GND | GND |
| Blu / SDA | D21 / GPIO21 |
| Giallo / SCL | D19 / GPIO19 |

Poi collega il secondo NeoKey al primo usando l'altro connettore STEMMA QT.

## Indirizzi I2C

Il primo NeoKey deve stare a `0x30`.
Il secondo NeoKey deve stare a `0x31`.

Se entrambi sono ancora a `0x30`, il bus vede due schede con lo stesso
indirizzo e non funziona bene. Cambia l'indirizzo del secondo saldando il
jumper address sulla scheda Adafruit per ottenere `0x31`.

In alternativa, collega SOLO il secondo NeoKey e carica lo sketch:

```text
ledTest/neokeySetSecondAddress031/neokeySetSecondAddress031.ino
```

Quando la seriale dice che risponde a `0x31`, spegni e riaccendi. Poi collega
entrambi i NeoKey in catena: il primo resta `0x30`, il secondo diventa `0x31`.

Nel codice:

```cpp
const uint8_t NEOKEY_ADDR_A = 0x30; // step 1-4
const uint8_t NEOKEY_ADDR_B = 0x31; // step 5-8
```

## Mappa step

| NeoKey | Tasto | Step sequencer |
| --- | --- | --- |
| primo, address 0x30 | sinistra 1 | step 1 |
| primo, address 0x30 | sinistra 2 | step 2 |
| primo, address 0x30 | sinistra 3 | step 3 |
| primo, address 0x30 | destra 4 | step 4 |
| secondo, address 0x31 | sinistra 1 | step 5 |
| secondo, address 0x31 | sinistra 2 | step 6 |
| secondo, address 0x31 | sinistra 3 | step 7 |
| secondo, address 0x31 | destra 4 | step 8 |

## LED

Da fermo:

| Colore | Significato |
| --- | --- |
| rosso | step attivo nel pattern |
| spento | rest |

In play:

| Colore | Significato |
| --- | --- |
| bianco | step corrente del clock |
| rosso | step attivo non corrente |

Il BPM resta mostrato sullo schermo LCD principale, non sui NeoKey: i NeoKey
non hanno display numerico, hanno solo i LED RGB sotto i tasti.
