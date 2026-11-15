## GrooveForge Bass 0.3.0 — versione preliminare

Sostituisce la 0.2.0: il generatore calibrato sui pacchetti di riferimento è stato ritirato.

### Novità
- **Generatore v3**: ripartito dalla 0.1.0 e calibrato solo sulle tue bassline fatte a mano. Rispetto alla 0.1.0:
  - note più lunghe e legate;
  - più movimento per gradi vicini (1–2 semitoni), pochi salti d'ottava;
  - frasi che ripetono un'idea di una battuta con piccole risposte;
  - registro più alto.
- **Nuove celle ritmiche**:
  - 12 celle prese dalle tue bassline;
  - 12 celle originali nello stile house / tech house groovy (swing, linee che rotolano, domanda e risposta).
  - Sono scritte da zero, non sono trascrizioni di brani esistenti.
- **LIKE / DISLIKE**: valuti ogni pattern sotto il piano roll e il generatore impara i tuoi gusti, in locale, a ogni voto.
  - Ritmi, intervalli e durate che ti piacciono diventano più probabili.
  - I ritmi che ti piacciono entrano nel tuo dataset personale.
  - DISLIKE passa subito a una nuova linea.
  - Il tasto **TASTE** spegne l'apprendimento.
  - In LIBRARY → TASTE trovi statistiche, l'esportazione dei pattern piaciuti come `.mid` e l'azzeramento del profilo.
- **Note cromatiche di avvicinamento** (opzionale, LIBRARY → ADVANCED).
- **Nuova grafica in stile svizzero** con tema scuro (predefinito) e chiaro, pulsante DARK in basso a destra: un solo rosso come accento, tipografia Helvetica/Arial, cursori lineari, griglia con filetti.
- **Installazione VST3 su Windows corretta**:
  - il plug-in va sempre in `C:\Program Files\Common Files\VST3`;
  - l'opzione "solo per me", che usava una cartella ignorata da molti DAW, è stata rimossa e la vecchia copia viene cancellata;
  - non serve più il Visual C++ Redistributable.
- Dopo l'installazione ripeti la scansione dei plug-in nel DAW. In Ableton attiva "Usa cartelle di sistema VST3".

I progetti salvati con le versioni precedenti si riaprono con le note esatte di allora. Gli stessi seed però generano pattern diversi.

Generatore di linee di basso + sintetizzatore SUB/ACID. VST3 per Windows e macOS, Audio Unit per macOS, applicazione standalone.

### Download

| Sistema | File | Contenuto |
|---|---|---|
| Windows 10/11 x64 | `GrooveForgeBass-0.3.0-win64-setup.exe` | Installer (amministratore): VST3 in `C:\Program Files\Common Files\VST3` + app standalone, con disinstallazione |
| Windows 10/11 x64 | `GrooveForgeBass-0.3.0-win64.zip` | Stessi file senza installer (script `install-vst3-windows.ps1` incluso) |
| macOS 11+ (Apple Silicon + Intel) | `GrooveForgeBass-0.3.0-macos-universal.dmg` | Installer `.pkg` con scelta VST3 / Audio Unit / app, guida e script di disinstallazione |

I file `.sha256` contengono le impronte SHA-256 per verificare i download.

### Cose da sapere prima di installare

- **Non firmati**: l'installer Windows non ha una firma Authenticode e il pacchetto macOS non ha Developer ID né notarizzazione Apple.
  - Windows: SmartScreen può avvisare → "Ulteriori informazioni" → "Esegui comunque".
  - macOS: clic destro sul `.pkg` → Apri → Apri.
- **Verificato in CI** con test automatici:
  - test unitari, di persistenza, del gusto e dell'interfaccia;
  - un harness host VST3 che carica il plug-in installato;
  - pluginval su Windows e auval per l'Audio Unit su macOS.
- **Non ancora provato a mano in un DAW.** Segnala i problemi nelle Issues.
- Costruito con JUCE 9.0.5 (AGPLv3 / licenza commerciale): vedi `LICENSE-THIRD-PARTY.md`.
