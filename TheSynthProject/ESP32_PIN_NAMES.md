# Nomi ufficiali dei pin per questo progetto

La scheda è la ESP32 DevKit V1 mostrata nella foto dell'hardware. In tutto
`TheSynthProject`, nei cablaggi e nelle conversazioni si usano esclusivamente i
nomi stampati sulla sua serigrafia.

Non aggiungere prefissi e non sostituire `RX2`, `TX2`, `RX0`, `TX0`, `VP` o
`VN` con numeri alternativi.

## Orientamento identico alla foto 5

Tenere la scheda con connettore USB in alto, antenna in basso e componenti
rivolti verso chi guarda.

### Lato sinistro, dall'alto verso il basso

`3V3`, `GND`, `D15`, `D2`, `D4`, `RX2`, `TX2`, `D5`, `D18`, `D19`, `D21`,
`RX0`, `TX0`, `D22`, `D23`.

### Lato destro, dall'alto verso il basso

`VIN`, `GND`, `D13`, `D12`, `D14`, `D27`, `D26`, `D25`, `D33`, `D32`,
`D35`, `D34`, `VN`, `VP`, `EN`.

## Nomi utilizzati dal cablaggio integrato

| Funzione | Serigrafia ESP32 |
| --- | --- |
| TFT CS | D5 |
| TFT DC | D4 |
| TFT RST | D2 |
| TFT SCK | D18 |
| TFT MOSI | D23 |
| Encoder master CLK | D32 |
| Encoder master DT | D33 |
| Encoder master SW | D25 |
| Tasto back | D26 |
| Bus principale SDA | D21 |
| Bus principale SCL | D22 |
| Bus MCP23017 SDA | D19, bus indipendente |
| Bus MCP23017 SCL | D15, bus indipendente |
| Libero con Daisy / PCM5102 BCK opzionale | D27 |
| Libero con Daisy / PCM5102 LCK opzionale | D14 |
| Libero con Daisy / PCM5102 DIN opzionale | D13 |
| UART da Daisy | RX2 |
| UART verso Daisy | TX2 |

I nomi degli altri componenti e il loro ordine fisico sono definiti in
[`COMPONENT_PIN_NAMES.md`](COMPONENT_PIN_NAMES.md).
