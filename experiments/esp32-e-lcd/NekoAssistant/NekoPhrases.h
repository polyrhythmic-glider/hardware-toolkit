#pragma once
#include <cstddef>
#include <cstdint>
#include <ctime>

// Short excerpts checked against linked sources on 2026-11-25. No runtime network calls.
// Editorial text is not copied; authors, source links and original wording are retained.
namespace NekoPhrases {
struct Entry { const char *text, *author, *source, *topic; };
inline constexpr Entry Entries[] = {
  {"Talk is cheap. Show me the code.", "Linus Torvalds", "https://it.wikiquote.org/wiki/Linus_Torvalds", "Hacker"},
  {"Odio gli indifferenti.", "Antonio Gramsci", "https://it.wikiquote.org/wiki/Antonio_Gramsci", "Antifascismo"},
  {"Non si ricordano i giorni, si ricordano gli attimi.", "Cesare Pavese", "https://it.wikiquote.org/wiki/Cesare_Pavese", "Poesia"},
  {"Free software is a matter of liberty, not price.", "Richard Stallman", "https://www.gnu.org/press/gnome-1.0.html", "Open source"},
  {"We live in capitalism, its power seems inescapable - but then, so did the divine right of kings.", "Ursula K. Le Guin", "https://www.ursulakleguin.com/nbf-medal", "Anticapitalismo"},
  {"E 'l naufragar m'è dolce in questo mare.", "Giacomo Leopardi", "https://it.wikisource.org/wiki/Canti_(1831)/L%27infinito", "Poesia"},
  {"Il miglior modo per predire il futuro è inventarlo.", "Alan Kay", "https://it.wikiquote.org/wiki/Alan_Kay", "Tecnologia"},
  {"La libertà è sempre la libertà di dissentire.", "Rosa Luxemburg", "https://www.fuoribinario.org/wp-content/uploads/2023/02/248-Fuori-Binario-copia-per-la-stampa.pdf", "Anticapitalismo"},
  {"My crime is that of curiosity.", "The Mentor", "https://phrack.org/issues/14/the-conscience-of-a-hacker-reprint", "Hacker"},
  {"L'attenzione è la forma più rara e più pura della generosità.", "Simone Weil", "https://it.wikiquote.org/wiki/Simone_Weil", "Filosofia"},
  {"Il vostro silenzio non vi proteggerà.", "Audre Lorde", "https://it.wikiquote.org/wiki/Audre_Lorde", "Controcultura"},
  {"Given enough eyeballs, all bugs are shallow.", "Eric S. Raymond", "https://en.wikiquote.org/wiki/Eric_S._Raymond", "Open source"},
  {"Ogni tempo ha il suo fascismo.", "Primo Levi", "https://it.wikiquote.org/wiki/Primo_Levi", "Antifascismo"},
  {"'Hope' is the thing with feathers -", "Emily Dickinson", "https://www.poetryfoundation.org/poems/42889/hope-is-the-thing-with-feathers-314", "Poesia"},
  {"Code is law.", "Lawrence Lessig", "https://www.harvardmagazine.com/2000/01/code-is-law-html", "Tecnologia"},
  {"Il vero è un momento del falso.", "Guy Debord", "https://it.wikiquote.org/wiki/Guy_Debord", "Anticapitalismo"},
  {"Gran parte del progresso sta nella volontà di progredire.", "Seneca", "https://it.wikiquote.org/wiki/Lucio_Anneo_Seneca", "Filosofia"},
  {"Stay hungry. Stay foolish.", "Whole Earth Catalog", "https://news.stanford.edu/stories/2005/06/youve-got-find-love-jobs-says", "Controcultura"},
  {"There is no justice in following unjust laws.", "Aaron Swartz", "https://pablorgarcia.github.io/open-access-manifesto/", "Hacker"},
  {"L'essenziale è invisibile agli occhi.", "Antoine de Saint-Exupéry", "https://it.wikiquote.org/wiki/Antoine_de_Saint-Exup%C3%A9ry", "Poesia"},
  {"La libertà è come l'aria.", "Piero Calamandrei", "https://www.lacostituzione.it/letture/calamandrei_1955.php", "Antifascismo"},
  {"Release early, release often.", "Eric S. Raymond", "https://en.wikiquote.org/wiki/Eric_S._Raymond", "Open source"},
  {"Né ridere, né piangere, ma capire.", "Baruch Spinoza", "https://it.wikiquote.org/wiki/Baruch_Spinoza", "Filosofia"},
  {"Sbagliando s'inventa.", "Gianni Rodari", "https://it.wikiquote.org/wiki/Gianni_Rodari", "Poesia"},
  {"Feminism is for everybody.", "bell hooks", "https://www.routledge.com/Feminism-Is-for-Everybody/Hooks/p/book/9781138821620", "Controcultura"},
  {"Sbagliando s'impara.", "Proverbio italiano", "https://it.wikiquote.org/wiki/Proverbi_italiani", "Quotidiano"},
  {"Quando insegnano, gli uomini imparano.", "Seneca", "https://it.wikiquote.org/wiki/Lucio_Anneo_Seneca", "Filosofia"},
  {"Chi ben comincia è a metà dell'opera.", "Proverbio italiano", "https://it.wikiquote.org/wiki/Proverbi_italiani", "Quotidiano"},
  {"Si vede solo con il cuore.", "Antoine de Saint-Exupéry", "https://it.wikiquote.org/wiki/Antoine_de_Saint-Exup%C3%A9ry", "Poesia"},
  {"Chi va piano va sano e va lontano.", "Proverbio italiano", "https://it.wikiquote.org/wiki/Proverbi_italiani", "Quotidiano"},
  {"Non ragioniam di lor, ma guarda e passa.", "Dante Alighieri", "https://it.wikiquote.org/wiki/Dante_Alighieri", "Poesia"},
  {"Chi cerca, trova.", "Proverbio italiano", "https://it.wikiquote.org/wiki/Proverbi_italiani", "Quotidiano"},
};
constexpr size_t Count = sizeof(Entries) / sizeof(Entries[0]);
constexpr uint32_t IntervalMs = 300000;
inline uint32_t offset = 0;
constexpr size_t indexAt(uint32_t now, uint32_t shift = 0) { return (now / IntervalMs + shift % Count) % Count; }
inline const Entry &current(uint32_t now) { return Entries[indexAt(now,offset)]; }
inline void next() { offset = (offset + 1) % Count; }
}
