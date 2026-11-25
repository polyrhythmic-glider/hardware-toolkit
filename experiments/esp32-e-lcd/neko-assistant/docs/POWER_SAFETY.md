# Energia, batteria e sicurezza

| Profilo | CPU | UI | Sensore | Wi‑Fi | BLE adv | Audio | Sleep |
|---|---|---|---|---|---|---|---|
| Performance | 80–240 MHz | 1 s se richiesto | 15 s | continuo | 250 ms | sì | no light sleep |
| Bilanciato | 80–160 MHz | 30 s/evento | 30 s | finestra 120 s | 500 ms obiettivo | sì | light sleep |
| Risparmio | 80–160 MHz | 60 s/evento | 120 s | finestra 30 s | 1 s obiettivo | no | light sleep |
| Notte | 80–160 MHz | 5 min/evento | 5 min | spento | 2 s obiettivo | no | light/deep |
| Critica | 40–160 MHz | finale | 10 min | spento | spento | no | deep sleep |

Neko misura GPIO4 con ADC eFuse quando disponibile, moltiplica per il rapporto ufficiale 3, media 16 conversioni e poi 12 campioni temporali. La curva 1S è configurabile nel sorgente e l’output è arrotondato al 5%. Non è un fuel gauge. Soglie con isteresi: critica 3,25/3,32 V, bassa 3,50/3,58 V, piena ≥4,14 V.

Usare soltanto una 18650 ricaricabile di qualità, polarità corretta, involucro integro. Il LED WRN acceso segnala inversione: scollegare subito. Non fare leva sul pannello quando si inserisce batteria/USB. Il firmware non può annullare il long-press hardware PWR durante OTA: l’utente deve mantenere alimentazione stabile; OTA è rifiutato sotto 50% salvo stima di carica.

Produzione: abilitare Secure Boot v2, Flash Encryption, NVS encryption e firmare le immagini. Il progetto abilita TLS CA, validazione immagine, dual OTA e rollback, ma le chiavi di produzione non sono incluse nel repository.
