# Architettura firmware

```text
driver/task ──> EventBus (queue 32) ──> AppController ──> CatStateMachine
   │                    │                    │                  │
   ├ board              ├ scheduler          ├ Time/Power       └ presentation
   ├ Wi‑Fi/BLE          └ notifications      └ UiModel ──> U8g2/ST7305
   └ audio/SD
```

Componenti ESP-IDF: `neko_events` non dipende da altri moduli Neko; `neko_board` possiede bus e periferiche; `neko_core` possiede policy, stato e persistenza; `neko_connectivity` possiede radio/OTA; `neko_ui` è l’unico proprietario del canvas; `main` compone gli oggetti. Non esistono dipendenze circolari.

| Task | Core | Priorità | Stack | Frequenza/blocco | Produce | WDT |
|---|---:|---:|---:|---|---|---|
| `main` / AppController | default IDF | 5 | config IDF | coda, max 1 s | UiModel/render | iscritto |
| `neko_input` | 1 | 5 | 3072 | 10 ms | eventi KEY | no, sempre cede |
| `neko_sensor` | 0 | 3 | 4096 | 15–600 s per profilo | T/RH + errori | no |
| `neko_battery` | 0 | 3 | 4096 | 30 s | tensione/band | no |
| `neko_timers` | 0 | 4 | 4096 | 250 ms | timer done | no |
| `neko_alarms` | 0 | 4 | 4096 | 1 s | alarm/reminder | no |
| `neko_audio` | 0 | 4 | 6144 | coda 4 | I²S | no, scritture finite |
| `neko_connect` | 0 | 3 | 4096 | una finestra | Wi‑Fi state | no |
| Wi‑Fi/NimBLE | sistema | sistema | SDK | eventi | radio | SDK |

Code: EventBus 32×`Event`, Audio 4×`ToneRequest`. Mutex: un mutex bus I²C, uno display, uno settings, uno per scheduler; ADC, SDMMC e I²S hanno proprietario singolo. Le chiamate lente non avvengono nella UI. Display SPI3 e SDMMC non condividono bus.

NVS: `neko_cfg/settings` contiene schema 2 e CRC32; `neko_sched/timers` e `alarms` sono blob transazionali. Password non viene mai loggata. Un blob corrotto ripristina default sicuri. Factory reset è intenzionalmente separato dal normale very-long press per evitare cancellazioni accidentali.

Safe mode: tre reset consecutivi per panic/WDT disabilitano radio e audio mantenendo display, RTC, sensore e diagnostica. Dopo 60 s stabili il contatore RTC viene azzerato.
