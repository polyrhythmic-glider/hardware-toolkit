# Piano di test e checklist hardware

Test host: `cmake -S tests/host -B build-host`, `cmake --build build-host`, `ctest --test-dir build-host`. Coprono curva batteria, CRC SHTC3 e BCD RTC.

Checklist su scheda:

- Flash: home leggibile in luce naturale, orientamento 400×300, nessun full refresh a 1 Hz nel profilo Bilanciato.
- KEY: 20 prove short, 10 double, 10 long, nessun rimbalzo; PWR conserva click-on/long-off hardware.
- RTC: avvio senza rete, OS flag dopo rimozione batteria RTC, SNTP e riscrittura, cambio ora legale Europe/Rome.
- SHTC3: confronto con riferimento, CRC forzato/NACK, offset termico dopo 30 min.
- Batteria: alimentatore 3,2–4,2 V, monotonia, isteresi, rumore, cutoff critico; misurare corrente con radio/audio/display per ogni profilo.
- SD: FAT32, assente al boot, rimozione durante idle, piena, file non valido, registrazione diagnostica ≤5 s.
- Audio: tono, mute/volume, speaker 8 Ω 2 W, entrambi i microfoni, file PCM e assenza di blocco UI.
- Wi‑Fi: password errata, AP assente, ritorno rete, RSSI, spegnimento dopo finestra, SNTP, HTTPS con certificato errato.
- BLE: pairing/bonding, reconnect dopo reboot, scrittura non cifrata rifiutata, provisioning chiuso rifiutato, limiti stringa, notifiche.
- Scheduler: quattro timer, pause/resume/reboot, otto sveglie, giorni settimana, snooze, promemoria scaduto, DST.
- Sleep: light sleep, wake KEY, deep sleep fino al prossimo evento tramite timer RTC ESP32, batteria critica.
- OTA: batteria <50% rifiutata, immagine corrotta, perdita rete, rollback, SD image troncata, power-loss manuale documentato.
- Recovery: SHTC3/RTC scollegato, codec assente, SD assente, boot-loop fino a safe mode, watchdog.

Per i consumi registrare tensione, corrente media/picco e durata di ogni fase; non pubblicare stime d’autonomia prima di misure con la cella target.
