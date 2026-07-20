# MCP23017 + un encoder, test isolato

Esperimento indipendente dal firmware `TheSynthProject`. Servono un secondo
ESP32, un solo MCP23017 e un solo encoder.

## MCP23017, lato controllo

| MCP23017 | Secondo ESP32 |
| --- | --- |
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SDA/SI` | `D21` |
| `SCL/SCK` | `D22` |
| `RESET` | direttamente `3V3` per il test |
| `A0` | `GND` |
| `A1` | `GND` |
| `A2` | `GND` |
| `NC/SO` | non collegato |
| `NC/CS` | non collegato |
| `ITA`, `ITB` | non collegati |

Aggiungere 4,7 kOhm tra `D21` e `3V3` e 4,7 kOhm tra `D22` e `3V3`.

## Encoder singolo

Non confondere `A0/A1/A2` del lato controllo con le porte A delle righe doppie.

| Encoder | MCP23017, porte I/O |
| --- | --- |
| `CLK` | `A0` della riga `B0/A0` |
| `DT` | `A1` della riga `B1/A1` |
| `SW` | `A2` della riga `B2/A2` |
| `+` | `3V3` |
| `GND` | `GND` comune |

## Uso

1. In Arduino IDE scegliere `ESP32 Dev Module`.
2. Aprire `mcp23017_single_encoder_test.ino` e caricarlo sul secondo ESP32.
3. Aprire il Monitor Seriale a `115200`, `New Line`.
4. Il test ripete automaticamente la scansione ogni due secondi finche' trova
   il MCP23017.

Risultato iniziale corretto:

```text
Devices: 0x20
MCP23017 0x20: OK
INITIAL RAW:0xFFFF CLK:HIGH DT:HIGH SW:HIGH COUNT:0
```

Anche senza encoder, ogni due secondi viene stampata una riga simile a:

```text
STATUS MCP:OK READS:2000 ERRORS:0 RAW:0xFFFF COUNT:0
```

Girando l'encoder compaiono righe `CHANGE` e `TURN`; premendo la manopola
compaiono `SW DOWN` e poi `SW UP`.
