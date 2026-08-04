# Control Surface Actions

Il TFT principale seleziona la traccia attiva. Il `PAGE MENU` dell'OLED E4
mostra quattro pagine contestuali, senza elencare le altre tracce.

Azioni del click sull'encoder E3 nella control surface OLED:

| Pagina | Azione E3 |
| --- | --- |
| Parametri/Oscillator V1 o V2 | Retrigger della voce attiva |
| Parametri/Shape Kick | Trigger kick |
| Sequencer | Toggle step on/rest della track attiva |
| Transport | Start/stop transport |

Quando la traccia TFT e' `KICK`, i tre mini OLED parametrici espongono la voce
909 della Daisy:

| Pagina OLED | E1 | E2 | E3 |
| --- | --- | --- | --- |
| `909 MAIN` | Level | Tune | Attack |
| `909 SHAPE` | Decay | Sweep | Drive |

La voce usa `DaisySP::SyntheticBassDrum`: Tune regola la fondamentale, Attack
il transiente, Decay la coda, Sweep l'inviluppo di pitch e Drive miscela la
saturazione conservando il percorso dry. E3 premuto fa sempre il preview del
kick nelle due pagine 909; E4 ruotato cambia pagina.

Note sequencer:

- Track `V1`: i NeoKey scrivono solo la sequenza V1.
- Track `V2`: i NeoKey scrivono solo la sequenza V2.
- Track `KICK`: i NeoKey scrivono solo la sequenza kick.
- I LED mostrano soltanto la corsia e la pagina da 16 step selezionate: V1
  rosso, V2 blu e kick verde. I colori delle corsie non vengono mai sommati.
- Per V1 e V2 il `PAGE MENU` contiene `OSCILLATOR 1` o `OSCILLATOR 2`. La
  pagina riepiloga Wave, Pitch, Tune, Fine, Result e Frequency.
- I NeoKey selezionano e attivano lo step nella pagina `Oscillator`. E1 assegna
  il Pitch MIDI assoluto allo step selezionato, E2 regola Tune in semitoni ed
  E3 regola Fine in cent. Result e Frequency vengono ricalcolati in tempo reale.
- Nella pagina OLED `Sequencer`, E1 seleziona lo step, E2 assegna anch'esso la
  nota assoluta e E3 imposta lo stato on/rest.
- In `Synth View`, la rotazione del master cambia traccia e il click commuta
  direttamente play/stop; il tasto back apre il menu raggruppato del sequencer.
