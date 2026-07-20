# Nomi e ordine fisico dei pin dei componenti

Questa è la legenda canonica ricavata esclusivamente dalle serigrafie nelle
foto. Fili, colori e collegamenti visibili sono ignorati. Quando si descrive un
cablaggio si mantiene sempre l'ordine riportato qui.

## Espansore I/O — orientamento della foto 1

Tenere il modulo con le scritte dritte come nella foto.

### Lato controllo, dall'alto verso il basso

1. `A2`
2. `A1`
3. `A0`
4. `RESET`
5. `NC/SO`
6. `NC/CS`
7. `SDA/SI`
8. `SCL/SCK`
9. `GND`
10. `VCC`

### Blocco doppio I/O, righe dall'alto verso il basso

In ogni riga il primo nome è quello prima della barra e il secondo è quello
dopo la barra, esattamente come stampato.

1. `VCC/GND`
2. `ITB/ITA`
3. `B0/A0`
4. `B1/A1`
5. `B2/A2`
6. `B3/A3`
7. `B4/A4`
8. `B5/A5`
9. `B6/A6`
10. `B7/A7`

Poiché `A0`, `A1` e `A2` compaiono sia sul lato controllo sia nel blocco I/O,
si specifica sempre anche il lato oppure la riga: per esempio `A0` del lato
controllo oppure `A0` della riga `B0/A0`.

## PCA9548A — orientamento della foto 2

Tenere la scheda con `PCA9548A` leggibile come nella foto.

### Lato sinistro, dall'alto verso il basso

1. `VIN`
2. `GND`
3. `SDA`
4. `SCL`
5. `RST`
6. `A0`
7. `A1`
8. `A2`
9. `SD0`
10. `SC0`
11. `SD1`
12. `SC1`

### Lato destro, dall'alto verso il basso

1. `SC7`
2. `SD7`
3. `SC6`
4. `SD6`
5. `SC5`
6. `SD5`
7. `SC4`
8. `SD4`
9. `SC3`
10. `SD3`
11. `SC2`
12. `SD2`

## Modulo TFT — orientamento della foto 3

Tenere il display rivolto verso chi guarda e la fila dei pin sul lato sinistro,
come nella foto. Dal basso verso l'alto, partendo dal foro inferiore:

1. `GND`
2. `VCC`
3. `SCL`
4. `SDA`
5. `RES`
6. `DC`
7. `CS`
8. `BLK`
9. `A`
10. `B`
11. `PUSH`
12. `K0`

## OLED — orientamento della foto 4

Tenere il display rivolto verso chi guarda, con la fila dei pin in alto. Da
sinistra verso destra:

1. `GND`
2. `VCC`
3. `SCL`
4. `SDA`

Lo stesso ordine vale per tutti e quattro gli OLED mostrati.

## PCM5102 — orientamento della foto

Tenere il modulo con la serigrafia leggibile come nella foto. Dall'alto verso
il basso:

1. `SCK`
2. `BCK`
3. `DIN`
4. `LCK`
5. `GND`
6. `VIN`

## ESP32 — orientamento della foto 5

Tenere USB in alto, antenna in basso e componenti rivolti verso chi guarda.

### Lato sinistro, dall'alto verso il basso

`3V3`, `GND`, `D15`, `D2`, `D4`, `RX2`, `TX2`, `D5`, `D18`, `D19`, `D21`,
`RX0`, `TX0`, `D22`, `D23`.

### Lato destro, dall'alto verso il basso

`VIN`, `GND`, `D13`, `D12`, `D14`, `D27`, `D26`, `D25`, `D33`, `D32`,
`D35`, `D34`, `VN`, `VP`, `EN`.

## Encoder rotativi

Per gli encoder si usa sempre l'ordine già specificato dall'utente:

1. `CLK`
2. `DT`
3. `SW`
4. `+`
5. `GND`
