# 4x NeoKey 1x4 QT nel firmware integrato

## Indirizzi

| Modulo | Indirizzo | Jumper |
| --- | --- | --- |
| step 1-4 | `0x30` | nessuno |
| step 5-8 | `0x31` | chiudere solo A0 |
| step 9-12 | `0x32` | chiudere solo A1 |
| step 13-16 | `0x33` | chiudere A0 e A1 |

## Collegamento in cascata sul canale 4

| Primo NeoKey/STEMMA QT | Collegamento |
| --- | --- |
| VIN rosso | 3V3 |
| GND nero | GND |
| SDA blu | PCA9548A SD4 |
| SCL giallo | PCA9548A SC4 |

Collegare gli altri tre NeoKey in cascata usando ogni presa STEMMA QT libera: i
cavi portano automaticamente alimentazione, SDA e SCL a tutti e quattro. Il
canale 5 resta libero. Tutti gli indirizzi devono essere diversi perché i
moduli sono sulla stessa catena.

L'ordine logico segue il montaggio fisico da sinistra verso destra: `0x30`
controlla gli step 1-4, `0x31` gli step 5-8, `0x32` gli step 9-12 e `0x33`
gli step 13-16. Anche i quattro tasti di ogni modulo avanzano da sinistra verso
destra. I tasti
controllano i sedici step del sequencer; la
selezione voce, le note e gli altri parametri restano disponibili sul TFT e
sulla control surface 4 OLED.

Cablaggio generale: [`WIRING_INTEGRATED.md`](WIRING_INTEGRATED.md).
