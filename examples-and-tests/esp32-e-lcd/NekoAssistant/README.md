# Neko · 0.7.0

Firmware per Waveshare ESP32-S3-RLCD-4.2 N16R8: pannello riflettivo **300×400**
usato in orizzontale a **400×300 pixel**, bianco e nero.

Il gatto è un personaggio originale ditherato, animato a partire da un'unica sprite:
corpo, mantello e proporzioni restano identici mentre si muovono coda, occhi e bocca.
Respira e reagisce alle coccole. Un fumetto collega la frase al muso.
Gli occhi a metà e chiusi usano palpebre curve dedicate: la pupilla non viene
deformata. Il movimento della coda sfuma fino al punto di attacco.
L'altra vista mostra soltanto l'orologio, con grandi cifre continue Orbitron.
Il display aggiorna solo le tile cambiate; il fumetto e i testi restano fermi.

## Schermate e comandi

La navigazione normale alterna solo **Neko** e **Orologio**, senza barra superiore,
riepiloghi o etichette decorative. La pagina di presentazione è stata eliminata.
Tutti i dati tecnici sono nel menu nascosto **Sistema**: Ambiente, Batteria,
Connessioni, SD, Audio, Preferenze e Diagnostica, più Esci.

| Gesto | Azione |
|---|---|
| Click KEY / GPIO18 oppure BOOT / GPIO0 dopo l'avvio | Alterna Neko e Orologio |
| KEY + BOOT insieme per 2 secondi | Apre/chiude Sistema |
| Click KEY / BOOT in Sistema | Voce successiva/precedente; da una sottopagina torna al menu |
| Doppio click in Sistema | Apre la voce selezionata oppure Esci |
| Doppio click su Neko | Coccola |
| Doppio click su Orologio | Altra frase |
| Doppio click su Timer | Pausa/riprende primo timer; crea 5 minuti se vuoto |
| Doppio click su Promemoria | Pausa tra 10 minuti, con ora valida |
| Doppio click su Messaggi | Segna come letto |
| Doppio click su Preferenze | Cambia profilo energetico |

Le pressioni lunghe di un solo tasto non aprono pagine tecniche.

BOOT durante accensione/reset seleziona il bootloader ESP32.

## Frasi e grafica

La frase cambia **ogni 5 minuti** dall'avvio e funziona offline; può essere cambiata
anche manualmente. Gli avvisi importanti hanno precedenza. Ogni citazione conserva autore
e link alla fonte. Alcune fonti:
[proverbi](https://it.wikiquote.org/wiki/Proverbi_italiani),
[Seneca](https://it.wikiquote.org/wiki/Lucio_Anneo_Seneca),
[Dante](https://it.wikiquote.org/wiki/Dante_Alighieri),
[Saint-Exupéry](https://it.wikiquote.org/wiki/Antoine_de_Saint-Exup%C3%A9ry),
[Pavese](https://it.wikiquote.org/wiki/Cesare_Pavese).
Non vengono eseguite chiamate di rete per le frasi.

32 citazioni con autore completo e fonte: hacker, open source, tecnologia, controcultura,
antifascismo, anticapitalismo, poesia, filosofia e proverbi. I temi si alternano;
i testi inglesi restano in originale. Fonti e note in `assets/quotes/SOURCES.md`.
Il fumetto adatta larghezza e altezza al testo, mantenendo un margine di 16 pixel
sopra e sotto; il piccolo logo Polyrhythmic Glider è fermo in alto a sinistra. Il fumetto usa
Helvetica Bold 18 (maiuscole alte 19 pixel, prima 10), interlinea 28 pixel e
Helvetica 10 per l'autore. Le citazioni sono misurate con il font reale U8g2:
nessun taglio, riduzione del carattere o autore abbreviato. Gli avvisi lunghi
possono usare Helvetica Bold 14.

Sprite originali create con **imagegen integrato**: sorgente, prompt e anteprime in
[`assets/pixel-neko/`](assets/pixel-neko/README.md). Il TTF originale Orbitron e la
licenza SIL OFL sono in `assets/orbitron/`. Il vecchio disegno ASCII non è utilizzato.

Rigenerare i formati embedded e web con Pillow:

```sh
python3 tools/pack_display_assets.py
```

L'importer produce una base 1-bit a 64/128/160/192/224 px, glifi dell'orologio e risorse web
incorporate. La dashboard usa la stessa base e la stessa macchina di animazione.

## Wi-Fi e dashboard

Aprire l'IP mostrato in Connessioni o `http://neko.local` sulla stessa LAN di Neko.
Utente `neko`, password casuale specifica del dispositivo nella pagina Connessioni.
La dashboard gestisce timer, promemoria, sveglie, messaggi, nome, profili energetici,
sincronizzazione ora e coccole; mostra il gatto animato con fumetto e fonte cliccabile.
Sprite e font sono serviti dall'ESP32: nessun CDN necessario.

Senza credenziali, Neko apre per 10 minuti una rete WPA2 `Neko-XXXX`, con la stessa
password, all'indirizzo `http://192.168.4.1`. Aprire Sistema con KEY+BOOT e scegliere
Connessioni riapre la configurazione per 2 minuti. Cambio rete consentito solo durante
questa finestra fisica.
HTTP con autenticazione locale: mantenere la dashboard nella propria LAN.

Trasferimento USB da una connessione NetworkManager già salvata, senza stampare
la password o scriverla nei sorgenti:

```sh
python3 tools/neko_usb.py --provision-from-nm NOME_CONNESSIONE
```

## Build e test

Arduino IDE 2.x: core ESP32 3.3.x, U8g2 2.36.19, ESP32S3 Dev Module,
USB Hardware CDC/JTAG e CDC On Boot, flash 16 MB QIO 80 MHz,
partizioni app3M_fat9M_16MB, PSRAM **OPI**. Aprire `NekoAssistant.ino`.
Il driver locale in `../rlcd_explorer` usa transazioni SPI bilanciate, compatibili
con i cambi di frequenza CPU.

```sh
./arduino-build.sh
./arduino-build.sh --upload /dev/ttyACM0
./tests/run-host.sh
./tests/run-typography.sh
python3 tests/test_display_assets.py
```

Lo script di build rileva Arduino CLI nel PATH o nell'IDE in `~/Scaricati`, scarica
U8g2 con SHA-256 verificato e usa il core ESP32 installato. Override:
`ARDUINO_CLI_BIN=/percorso/arduino-cli`. Build e cache: `.build/neko/` nel repository.

I test verificano gesti e combinazione nascosta, navigazione a due viste, scheduler,
aggiornamenti parziali, identità della sprite fuori dalle zone animate, moto continuo,
400 campioni identici fra C++ e JavaScript, cambio frase a 5 minuti, tutti i 1.440
orari, continuità dei glifi e asset nativi a un bit. L'ultimo collaudo reale è
registrato in `BUILD_VERIFIED.md`.

## USB e limiti

Seriale a 115200: `help`, `status`, `home`, `cat`, `clock`, `phrase-next`, `pet`,
`diag`, `timer <secondi>`, `pause`, `resume`, `cancel`, `wifi`, `screen`.

```sh
python3 tools/neko_usb.py --command status
python3 tools/neko_usb.py --capture /tmp/neko-screen.png
```

Il tool USB richiede pyserial, le catture anche Pillow. `screen` esporta il framebuffer
reale; non ricostruisce un mockup della schermata.

- Fino a quattro timer, otto sveglie, quattro promemoria persistenti.
- Timer salvati ogni minuto e ripristinati in pausa dopo un riavvio.
- Orologio: RTC hardware, sincronizzazione SNTP o da companion.
- Batteria e stato di carica stimati dalla tensione; deep sleep a livello critico.
- Codec audio rilevati, ma avvisi visivi e nessun assistente vocale incluso.
- SD update: `neko-update.bin` + `neko-update.ready`, con almeno 50% di batteria
  o carica stimata. Dopo tre crash: safe mode; conteggio azzerato dopo un minuto stabile.
