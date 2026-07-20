# Protocollo BLE Neko 1.0

Tutti gli interi sono little-endian. Stringhe UTF‑8 senza terminatore. Limite applicativo 192 byte; password max 64, SSID 32, nome 23, notifica 95. Le scritture richiedono cifratura BLE; Wi‑Fi richiede anche finestra fisica KEY. Pairing Secure Connections e bonding sono attivi; non essendoci tastiera, 1.0 usa Just Works e non dichiara MITM.

Servizi standard: Battery `0x180F/0x2A19` (`uint8 %`), Environmental Sensing `0x181A` con temperatura `0x2A6E` (`int16`, 0,01 °C) e umidità `0x2A6F` (`uint16`, 0,01 %RH), Device Information `0x180A` con Model/Firmware.

Servizio custom: `01004f4b-454e-a19e-6f4d-5280106e2b7a`. Le caratteristiche sostituiscono il primo byte (`10`…`1b`) nella forma canonica.

| ID | Nome | Proprietà | Formato |
|---:|---|---|---|
| 10 | Protocol | R | `{major:u8, minor:u8}` = 1.0 |
| 11 | Status | R,N | version, flags, battery, reserved, temp i16, RH i16, RSSI i16, epoch u32 |
| 12 | Time | R,W,ENC | Unix epoch u64 |
| 13 | Timezone | R,W,ENC | regola POSIX, max 63 |
| 14 | Alarm | W,ENC | hour, minute, weekday mask (bit0=dom), enabled, label |
| 15 | Timer | W,ENC | seconds u32 + label |
| 16 | Notification | W,ENC | testo UTF‑8 |
| 17 | Cat name | R,W,ENC | UTF‑8 |
| 18 | Wi‑Fi | W,ENC,AUTHOR | ssid_len u8, pass_len u8, SSID, password |
| 19 | Volume | R,W,ENC | u8 0…100 |
| 1a | Control | W,ENC | 1 find, 2 reboot, 3 diagnostics, 4 OTA request |
| 1b | Diagnostics | R | JSON compatto, max 160 |

Errori ATT: lunghezza invalida, valore non consentito, cifratura insufficiente, autorizzazione fisica insufficiente, risorsa esaurita. La companion di riferimento è in `companion/`.
