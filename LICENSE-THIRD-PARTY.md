# Third-party software and licences

## JUCE 9.0.5
- Source: https://github.com/juce-framework/JUCE, tag `9.0.5` (commit `d60b7a78030661574a615b586d9325ba7a7fa7dd`), fetched by CMake FetchContent or provided via `GFB_JUCE_DIR`.
- Licence: dual — **AGPLv3** or the commercial **JUCE 9 EULA** (https://juce.com/legal/juce-9-licence/).
- Consequence: a closed-source / commercial release of GrooveForge Bass requires a commercial JUCE licence. Without it, any distributed binary must comply with the AGPLv3 (source availability etc.). No licence decision has been made yet; JUCE 9 no longer has a splash screen.

## Steinberg VST3 SDK (bundled inside JUCE)
- Used through JUCE's VST3 wrapper/hosting code. Licence: the VST3 SDK is distributed under GPLv3 or the Steinberg proprietary licence (see JUCE `modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt`). "VST" is a trademark of Steinberg Media Technologies GmbH.

## pluginval (CI only, not shipped)
- https://github.com/Tracktion/pluginval (GPLv3). Downloaded only inside the GitHub Actions job to validate the built bundle.

## Original material in this repository
- All C++ source, the rhythm archetype corpus (`src/core/RhythmCorpus.cpp`), fill cells, synth presets, UI design and documentation were written for this project. No third-party songs, MIDI packs, sample libraries, Bassurito data or assets were used.
