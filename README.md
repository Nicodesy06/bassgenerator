# GrooveForge Bass

Generatore di linee di basso + sintetizzatore di basso integrato, plugin **VST3 strumento** per **Windows 10/11 x64** (build anche per Linux per sviluppo/CI). Implementazione originale in C++20 + JUCE 9.0.5. Nome provvisorio.

![GrooveForge Bass](docs/images/editor_sub_1180x760.png)

## Cosa fa

- **GENERATE**: un clic crea una frase di basso musicale (1/2/4/8 battute, 4/4) secondo Tonalità, Scala, Famiglia, Feel, Battute e Ottava.
- Motore di generazione **deterministico e offline**: corpus originale di 57 archetipi ritmici scritti a mano, grammatica ritmica con mutazioni vincolate, traiettoria di pitch condizionata sul ruolo ritmico, struttura di frase (A A' B F…), valutazione a candidati. Niente cloud, niente "AI".
- **Due seed**: `STYLE` (idea: archetipo, motivo, frase) e `VAR` (variazioni limitate della stessa idea). Modificabili con trascinamento (Shift = fine), rotella, frecce, doppio clic per digitare, tasto destro copia/incolla.
- **Lock** per ogni impostazione e per i seed; **History** delle ultime 64 generazioni (snapshot esatti); **Favorites** globali e persistenti, condivisi tra istanze.
- **Piano roll** che mostra esattamente il MIDI esportato (swing incluso), con playhead, velocity, accenti e slide.
- **Sintetizzatore** monofonico/legato con due motori: **SUB** (sinusoide pulita + strato armonico filtrato) e **ACID** (saw/square in ladder 24 dB risonante con accent e slide). Otto macro: Volume, Cutoff, Resonance, Env Mod, Decay, Accent, Drive, Glide (+ Shape). 8 preset originali.
- **Esporta MIDI** (.mid SMF tipo 0, PPQ 960, tempo e 4/4) e **trascina** il pattern direttamente su una traccia del DAW. Opzione **MIDI OUT** VST3 (dipende dall'host).
- Sync al transport del DAW oppure **PREVIEW** a transport fermo.

## Requisiti di build (Windows)

- Windows 10/11 x64, **Visual Studio 2022** con il workload "Sviluppo di applicazioni desktop con C++" (MSVC + Windows SDK)
- **CMake ≥ 3.24**, Git, PowerShell
- Connessione internet alla prima configurazione (JUCE 9.0.5 viene scaricato via FetchContent). Per build offline: clonare JUCE `9.0.5` e impostare `GFB_JUCE_DIR`.

## Build, test, pacchetto (Windows)

```powershell
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release --output-on-failure
```

Oppure con gli script (log in `artifacts\logs`):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1
powershell -ExecutionPolicy Bypass -File scripts\test-windows.ps1 -Pluginval C:\tools\pluginval.exe
powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
```

Output:

- VST3: `build\windows-x64\GrooveForgeBass_artefacts\Release\VST3\GrooveForge Bass.vst3` (cartella-bundle, non solo una `.dll`)
- Standalone: `build\windows-x64\GrooveForgeBass_artefacts\Release\Standalone\GrooveForge Bass.exe`
- Pacchetto: `dist\GrooveForgeBass-0.1.0-win64.zip` + `.sha256`

La CI GitHub Actions (`.github/workflows/windows.yml`) compila e testa su `windows-2022` e pubblica lo ZIP come artifact.

## Installazione

- **Solo utente corrente (senza admin)**: `scripts\install-user-windows.ps1` copia il bundle in `%LOCALAPPDATA%\Programs\Common\VST3\`.
- **Tutti gli utenti (admin)**: copiare la cartella `GrooveForge Bass.vst3` in `C:\Program Files\Common Files\VST3\`.

Poi rieseguire la scansione dei plugin nel DAW e inserire "GrooveForge Bass" come strumento su una traccia.

## Uso rapido

1. Scegli Key, Scale, Family (SUB/ACID), Feel, Bars. Premi **GENERATE** (tasto `G`).
2. Premi Play nel DAW (SYNC attivo) oppure **PREVIEW** (`P`) a transport fermo.
3. Ti piace l'idea ma vuoi varianti? **NEW VAR** (`V`) mantiene lo STYLE. Vuoi un'altra idea? **NEW STYLE** (`S`). **RANDOMISE** (`R`) cambia tutto ciò che non è bloccato.
4. Torna indietro con `<` / `>` (o Ctrl+Z / Ctrl+Y). Salva con **+ FAV** (`F`). LIBRARY apre History, Favorites e opzioni avanzate (swing, contesto ritmico della cassa, BPM di fallback, reset).
5. Trascina il piano roll (o il pulsante DRAG) su una traccia MIDI del DAW, oppure **EXPORT** per salvare un `.mid`.

I Favorites sono in `%APPDATA%\GrooveForgeBass\favorites.json` (con backup `favorites.bak.json`).

## Stato e limiti (onestà)

- Il binario **Windows** è prodotto e testato **solo dalla CI** su GitHub Actions; non è stato provato a mano in un DAW Windows. Vedi `docs/QUALITY_REPORT.md` e `docs/DAW_COMPATIBILITY.md`.
- La qualità musicale è misurata con statistiche (`reports/`), ma **non è ancora stata validata con test d'ascolto** di produttori: il protocollo è in `docs/TEST_PLAN.md`.
- Nessun legame con Bassurito: codice, corpus, suoni, nome e grafica sono originali.

## Licenze

JUCE è dual-licensed **AGPLv3 / commerciale**. Distribuire binari closed-source richiede una licenza JUCE commerciale; altrimenti il progetto va distribuito sotto AGPLv3. Dettagli in `LICENSE-THIRD-PARTY.md`.

Documentazione tecnica (inglese): `docs/ARCHITECTURE.md`, `docs/ALGORITHM.md`, `docs/TEST_PLAN.md`, `docs/QUALITY_REPORT.md`, `docs/DAW_COMPATIBILITY.md`, `docs/PLAN.md`, `CLAUDE.md`.
