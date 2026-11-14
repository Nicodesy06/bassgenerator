## GrooveForge Bass 0.1.0 — versione preliminare

Generatore di linee di basso + sintetizzatore SUB/ACID. VST3 per Windows e macOS, Audio Unit per macOS, applicazione standalone.

### Download

| Sistema | File | Contenuto |
|---|---|---|
| Windows 10/11 x64 | `GrooveForgeBass-0.1.0-win64-setup.exe` | Installer: VST3 (`C:\Program Files\Common Files\VST3`, oppure per utente) + app standalone, con disinstallazione |
| Windows 10/11 x64 | `GrooveForgeBass-0.1.0-win64.zip` | Stessi file senza installer (copia manuale) |
| macOS 11+ (Apple Silicon + Intel) | `GrooveForgeBass-0.1.0-macos-universal.dmg` | Installer `.pkg` con scelta VST3 / Audio Unit / app, guida e script di disinstallazione |

I file `.sha256` contengono le impronte SHA-256 per verificare i download.

### Cose da sapere prima di installare

- **Non firmati**: l'installer Windows non ha una firma Authenticode e il pacchetto macOS non ha Developer ID né notarizzazione Apple.
  - Windows: SmartScreen può avvisare → "Ulteriori informazioni" → "Esegui comunque".
  - macOS: clic destro sul `.pkg` → Apri → Apri, oppure Impostazioni di Sistema → Privacy e sicurezza → "Apri comunque".
- **Verificato** in CI con test automatici: 18 test, un harness host VST3 che carica il plug-in installato, pluginval strictness 10 su Windows e auval per l'Audio Unit su macOS. Installazione e disinstallazione sono state provate in modalità silenziosa sulle macchine CI.
- **Non ancora provato a mano in un DAW**: REAPER, Ableton, Logic, FL Studio, Bitwig. Il drag & drop del MIDI e il MIDI OUT dipendono dall'host. Segnala i problemi nelle Issues.
- La qualità musicale non è ancora stata valutata da test d'ascolto.
- Costruito con JUCE 9.0.5 (AGPLv3 / licenza commerciale): vedi `LICENSE-THIRD-PARTY.md`.
