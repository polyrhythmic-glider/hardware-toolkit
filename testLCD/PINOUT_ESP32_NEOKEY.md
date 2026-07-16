# Pinout ESP32 DevKit V1 per 2x NeoKey 1x4 QT

Questo e il pinout attivo del firmware dopo la rimozione del modulo
`TM1638 LED&KEY`.

Con USB in basso e antenna/modulo ESP-32 in alto:

## Pin usati dal codice adesso

| Funzione | Serigrafia ESP32 | GPIO reale |
| --- | --- | --- |
| NeoKey SDA | D21 | GPIO21 |
| NeoKey SCL | D19 | GPIO19 |
| NeoKey VIN | 3V3 | 3.3V |
| NeoKey GND | GND | GND |
| TFT CS | D5 | GPIO5 |
| TFT DC | D4 | GPIO4 |
| TFT RST | D2 | GPIO2 |
| TFT SCK | D18 | GPIO18 |
| TFT MOSI | D23 | GPIO23 |
| Encoder A | D32 | GPIO32 |
| Encoder B | D33 | GPIO33 |
| Encoder SW | D25 | GPIO25 |
| K0 button | D26 | GPIO26 |
| PCM5102 BCK | D27 | GPIO27 |
| PCM5102 LRCK | D14 | GPIO14 |
| PCM5102 DIN | D22 | GPIO22 |

## NeoKey

| Modulo | Indirizzo I2C | Step |
| --- | --- | --- |
| primo NeoKey 1x4 | 0x30 | 1-4 |
| secondo NeoKey 1x4 | 0x31 | 5-8 |

I due NeoKey non possono avere lo stesso indirizzo. Il secondo va cambiato a
`0x31` con il jumper address sulla scheda.

## Pin da lasciare scollegati per NeoKey

| Serigrafia | Motivo |
| --- | --- |
| TX2 / GPIO17 | non serve piu, era DIO TM1638 |
| RX2 / GPIO16 | non serve piu, era OE level shifter TM1638 |
| D13 / GPIO13 | non serve |

## Cablaggio STEMMA QT

| Filo STEMMA QT | ESP32 |
| --- | --- |
| VIN rosso | 3V3 |
| GND nero | GND |
| SDA blu | D21 / GPIO21 |
| SCL giallo | D19 / GPIO19 |
