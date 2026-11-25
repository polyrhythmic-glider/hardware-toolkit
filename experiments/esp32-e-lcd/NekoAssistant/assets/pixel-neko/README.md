# Neko Pixel Pet

Originale generato con lo strumento **imagegen integrato**, 24 novembre 2026.
Nessuna mascotte esistente è stata usata come immagine di riferimento.

- `source-atlas.png`: output originale ImageGen, 1536×1024, otto pose in griglia 4×2.
- `PROMPT.txt`: prompt completo usato per la generazione.
- `pet-atlas.png`: unica base di produzione 1-bit, 160×160 (nome mantenuto per compatibilità).
- `preview.png` / `preview.gif`: anteprime della conversione per il display.

`../../tools/pack_display_assets.py` estrae soltanto la prima cella del sorgente,
con crop fisso e dithering Bayer deterministico. Le altre sette pose generate
sono conservate solo come sorgente e non vengono usate: variavano le proporzioni.
Le versioni native 64/128/160/192/224 px sono incorporate in `../../NekoPetAssets.h`;
il web serve la stessa base a 160 px.

`../../NekoPet.h` anima questa singola base con piccoli spostamenti della coda,
palpebre, bocca, respiro e un cuore durante le coccole. Le palpebre a metà coprono
la pupilla originale con un tratto curvo; quelle chiuse hanno curve e ciglia
dedicate alla prospettiva di ogni occhio. Corpo, macchie e collare
non cambiano fra i frame. I test confrontano ogni pixel fuori dalle zone animate
e 400 campioni C++/JavaScript. Il movimento continua anche nei profili notte e risparmio.

Il font dell'orologio è Orbitron, scaricato dal repository ufficiale Google Fonts:
https://github.com/google/fonts/tree/main/ofl/orbitron
La licenza SIL OFL e il TTF originale sono in `../orbitron/`.
