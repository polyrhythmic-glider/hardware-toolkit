# Neko 0.7.0 — 25 novembre 2026

Firmware compilato e caricato via USB sulla Waveshare ESP32-S3-RLCD-4.2 N16R8,
300×400 fisici, orientamento orizzontale **400×300**, display a un bit.

- Core ESP32 3.3.11, U8g2 2.36.19; flash 16 MB e PSRAM OPI.
- Programma: **1.484.955 byte / 3.145.728 (47%)**.
- Variabili globali: **77.576 byte / 327.680 (23%)**.
- Upload ROM USB 460800 baud, `--no-stub`; hash flash verificato.
- SHA-256 applicazione: `7d59409ffd618644de47c185caf1fbd1904e4ea40958061188c0c7f173eb0406`.

## Interfaccia

- Due viste normali: Neko e Orologio. Nessuna barra superiore o pagina di presentazione.
- Orologio: solo HH:MM con cifre continue Orbitron, senza logo o altri elementi.
- Neko: sprite ditherata nativa 192 px, con un'unica identità e dettagli immutati.
- Mezzo blink: palpebra curva sopra la pupilla originale, senza comprimerla;
  occhi chiusi: curve e ciglia dedicate alla prospettiva dei due occhi.
- Coda con movimento che sfuma al punto di attacco; bocca senza il vecchio anello.
- Logo Polyrhythmic Glider fornito dall'utente, 160×40 a x=12/y=16, solo su Neko.
- Fumetto adattivo: nelle 32 citazioni provate, larghezza 157–203 px e altezza
  88–256 px. Misure derivate dalle metriche effettive del font.
- Helvetica Bold 18, maiuscole alte 19 px (prima 10), interlinea 28 px;
  autore Helvetica 10 completo. Nessuna citazione viene ridotta o troncata.
- 32 estratti su 9 temi: hacker, open source, tecnologia, controcultura,
  antifascismo, anticapitalismo, poesia, filosofia e proverbi, ogni **5 minuti**.
  Autore e fonte per ogni voce in `NekoPhrases.h`; inventario in `assets/quotes/SOURCES.md`.
- Diagnostica nel menu Sistema, accessibile con KEY+BOOT tenuti 2 secondi.

## Collaudo

- Test host: pulsanti, combinazione nascosta, navigazione, scheduler, persistenza,
  aggiornamenti parziali, continuità dei glifi dell'orologio, rotazione a 300.000 ms.
- Test degli occhi: la pupilla visibile nel mezzo blink conserva i pixel originali;
  le palpebre chiuse hanno curvatura, mentre corpo, naso e baffi restano invariati.
- **400 animazioni C++/JavaScript identiche bit per bit**.
- Test tipografico con il font U8g2 reale: tutte le 32 citazioni e gli autori
  completi ricostruiti senza perdite; glifi disponibili, testo intero dentro i bordi.
- Framebuffer reale: **40 frame Neko**, con pixel fuori dal gatto identici.
- **Tutte e 32 le nuvolette acquisite e confrontate con le geometrie attese**:
  nessun pixel fuori dalle aree previste; offset citazione ripristinato a fine prova.
- Logo confrontato pixel per pixel con l'asset convertito dal PNG dell'utente.
- Due catture Orologio: nessun contenuto esterno all'area delle cifre.
- USB `diag` non apre il menu senza combinazione fisica; la combinazione è coperta
  dai test host e già confermata dall'utente nella versione precedente.
- Dashboard Wi-Fi autenticata su muga: versione 0.7.0, sprite PNG, font Orbitron
  originale, metadati citazione e rimozione delle vecchie etichette verificati.
  Rete precedente del computer ripristinata al termine.
- `git diff --check`: superato. Nessuna password nei sorgenti o nei report.

Build, log, screenshot e GIF del display reale: `.build/neko/v7/` (ignorata da Git).
Le verifiche grafiche confrontano il framebuffer; non sono misure strumentali del pannello.

---

## Storico 0.4 e precedenti

# Neko Pixel Pet 0.4.0 — 24 novembre 2026

Target: Waveshare ESP32-S3-RLCD-4.2 N16R8, 4,2 pollici, 300×400 fisici,
orientamento orizzontale U8g2 R1 a **400×300 pixel**, un bit.

- Arduino CLI 1.5.0, core ESP32 3.3.11, U8g2 2.36.19.
- Programma: **1.508.707 byte / 3.145.728 (47%)**.
- Variabili globali: **71.280 byte / 327.680 (21%)**.
- Upload USB ROM 460800 baud; hash flash verificato.
- SHA-256 applicazione: `de98c963c07bad2ab7e9ac9b48e160ed65f474a588f8b1d31c8393382e91c583`.

## Risultato

- Personaggio originale ImageGen, otto pose ditherate in formato nativo 1-bit,
  risoluzioni 64/128/160. Sorgente e prompt in `assets/pixel-neko/`.
- Respiro, coda, blink, parlato, coccole e allerta; movimento anche in modalità notte.
- Frase del giorno nel fumetto collegato al muso, con autore e fonte.
- Cifre continue Orbitron: test pixel conferma un componente connesso per cifra,
  non segmenti LED separati. Licenza e font originale in `assets/orbitron/`.
- Eliminate le etichette decorative indicate dall'utente, anche dalla dashboard.
- Pagina di presentazione eliminata da enum, titoli, renderer e ciclo di navigazione.

## Verifica

- Tutti i test host superati: pulsanti, scheduler, geometria di tutti i 1.440 orari,
  asset binari, glifi connessi, animazione continua, frasi e aggiornamenti parziali.
- 400 campioni di animazione identici fra C++ e JavaScript.
- Framebuffer reali acquisiti: 24 Home, 16 Gatto, 16 Orologio.
- Su tutte e tre le schermate, i pixel esterni al gatto rimangono identici durante
  l'animazione. Catture/GIF e report in `.build/neko/v4/`.
- La prova riporta in primo piano la pagina da acquisire prima di ogni frame;
  nessun GPIO, timer o dato utente viene modificato.
- Collegamento Wi-Fi su muga, con impostazioni conservate.

Backup dell'applicazione precedente: `.build/neko/v4/previous-firmware.bin`.
Nessuna password o chiave è presente nei sorgenti o nei report.

---

## Storico precedente

# Neko ASCII Edition 0.3.0 — verifica del 24 novembre 2026

Build eseguita e caricata sulla scheda USB ESP32-S3 N16R8 (MAC terminante 59:90):

- Arduino CLI 1.5.0, core ESP32 3.3.11, U8g2 2.36.19 (SHA-256 della libreria verificato).
- Flash 16 MB, OPI PSRAM, partizioni app3M_fat9M_16MB, CDC Hardware/JTAG.
- Programma: **1.392.231 byte / 3.145.728 (44%)**.
- Variabili globali: **71.208 byte / 327.680 (21%)**.
- Upload USB in modalità ROM `--no-stub`, 460800 baud; verifica hash flash superata.
- SHA-256 applicazione: `54b213b0bbf15eff59546d3d0854748c220a99b9ec3113ae0318be18dcbac344`.

## Orologio cubitale e frasi verificate online

- Cifre a sette segmenti native, alte **184 pixel**, impaginazione larga **364 pixel**.
- Test di tutti i 1.440 orari validi: nessuna geometria fuori dall'area prevista.
- Ora non sincronizzata: trattini; i due punti non lampeggiano.
- Dieci brevi citazioni e proverbi verificati su Wikiquote, con autore e URL in
  `NekoPhrases.h`. Rotazione ogni minuto, disponibile anche offline.
- Test della rotazione automatica, cambio manuale, metadati e unicità delle frasi.
- Verifica del framebuffer reale: tutti i segmenti nell'area prevista e altezza
  effettiva di 184 pixel. Immagine in `.build/neko/clock-large.png`.
- Dashboard reale via Wi-Fi: fonte/autore della frase, cambio frase e comando
  Orologio verificati. Due catture USB confermano la visualizzazione su Orologio
  e Home. Report in `.build/neko/large-clock-verification.json`.
- Rete del computer ripristinata a Pixel_bak dopo il collaudo su muga.
- Copia della build precedente: `.build/neko/neko-before-large-clock.bin`.

## Aggiornamento del gatto scelto dall'utente

Riferimento: [Cat, ASCII Art Archive](https://www.asciiart.eu/art/8e03bfe9f5b3c218),
autore sconosciuto. Il frame iniziale riproduce le quattro righe originali,
con 23 colonne di larghezza massima. Coda, occhi, orecchio e zampa sono animati.

- Fedeltà del frame iniziale e confronto di **430 frame** C++/JavaScript: superati.
- Nuova build caricata via USB con verifica dell'hash flash.
- Home e pagina Gatto acquisite dal framebuffer reale, senza sovrapposizioni.
- Dieci frame Home: pixel esterni all'area gatto identici.
- Dashboard Wi-Fi autenticata: verificato che serva lo stesso disegno di riferimento.
- Conferma dell'utente ricevuta sulla stabilità della UI prima di questa modifica;
  il meccanismo di aggiornamento parziale rimane lo stesso.
- Copia locale della build stabile precedente al nuovo gatto:
  `.build/neko/neko-before-crouching-cat.bin`.
- Anteprime reali: `.build/neko/crouching-home.gif` e `.build/neko/crouching-cat.gif`.

## Prove eseguite

- Test host di debounce, gesti, rollover, frame ASCII, aggiornamento display parziale,
  timer simultanei, pausa/ripresa, persistenza, promemoria e sveglie: superati.
- Sintassi JavaScript della dashboard incorporata e della companion BLE: verificata.
- Avvio reale, risposta USB, SHTC3 e batteria: verificati; microSD assente.
- Connessione Wi-Fi 2,4 GHz e mDNS: verificati.
- Dashboard HTTP, autenticazione, lettura stato e sincronizzazione ora: verificate.
- Cambio e ripristino di tutti e quattro i profili energetici via HTTP: verificati,
  senza il precedente blocco nel callback SPI al cambio della frequenza CPU.
- POST da origine diversa: respinto con HTTP 403.
- Confronto di 10 framebuffer Home reali: **cambiano solo i pixel nell'area gatto**;
  tutti i pixel esterni sono identici. Acquisite anche 24 immagini della pagina Gatto aggiornata.
- Quattro timer già memorizzati occupavano gli slot: il test HTTP di creazione di un
  timer temporaneo è stato saltato; lo scheduler è coperto dai test host.
- Non verificati in questa sessione: browser grafico (host non disponibile), interazione
  BLE end-to-end, SD/OTA, riproduzione audio e misura strumentale dei consumi.

La copia precedente della partizione app0 è conservata localmente in
`.build/neko/original-app0.bin` (3 MB; NVS/credenziali escluse), SHA-256:
`d3277942f7e9cefae6304e9ddb1c5f8a15cb3f1a05e68d53f60edd8abaa3ed60`.
Le prove e i framebuffer sono in `.build/neko/`, ignorata da Git. Nessuna password
Wi-Fi o chiave della dashboard è stata scritta nei sorgenti o nei report.

---

## Build storica Arduino 0.2.0


Verifica eseguita il **15 luglio 2026** con:

- Arduino CLI 1.5.1 (quello incluso in Arduino IDE);
- `esp32:esp32` 3.3.10;
- board `ESP32S3 Dev Module`;
- U8g2 2.36.19 installata nell'Arduino IDE;
- Flash 16 MB, QIO 80 MHz, OPI PSRAM, doppia partizione OTA da 3 MB.

FQBN usato:

```text
esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadMode=default
```

Esito:

```text
Sketch uses 1287753 bytes (40%) of program storage space. Maximum is 3145728 bytes.
Global variables use 52112 bytes (15%) of dynamic memory, leaving 275568 bytes for local variables. Maximum is 327680 bytes.
```

Artefatti:

| File | SHA-256 |
|---|---|
| `build-key-loop-task-v2/NekoAssistant.ino.bin` | `5F098B45557A9E387DF8B8A20EAAD71302A2A0F687DCE386E952980F76217D4E` |
| `build-key-loop-task-v2/NekoAssistant.ino.merged.bin` | `AE4CB53B43600E13428E4ACD544AE798F91F2FFFD1477D779CE7D995E123DE3D` |

La compilazione non sostituisce il collaudo sulla scheda fisica: display, I2C,
ADC, SD, RF, sleep/wake e OTA devono essere provati seguendo il test plan.
