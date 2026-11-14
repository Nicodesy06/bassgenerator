cmake_minimum_required(VERSION 3.24)

project(GrooveForgeBass VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# No GNU extensions: keeps -ffp-contract=off on GCC so floating point in the
# generator/evaluator is reproducible across toolchains (see docs/ALGORITHM.md).
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GFB_BUILD_PLUGIN "Build the JUCE VST3/Standalone plugin" ON)
option(GFB_BUILD_TESTS  "Build unit/integration tests"           ON)
option(GFB_BUILD_TOOLS  "Build diagnostics/render tools"         ON)
set(GFB_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (tag 9.0.5). Empty = FetchContent download.")
set(GFB_JUCE_TAG "9.0.5" CACHE STRING "Pinned JUCE git tag")

if(MSVC)
    add_compile_options(/utf-8 /fp:precise)
endif()

# ---------------------------------------------------------------------------
# Pure C++ core: generator, theory, DSP, SMF writer. No JUCE dependency.
# ---------------------------------------------------------------------------
add_library(gfb_core STATIC
    src/core/PatternTypes.cpp
    src/core/GeneratorSettings.cpp
    src/core/ScaleTheory.cpp
    src/core/SeedRng.cpp
    src/core/RhythmCorpus.cpp
    src/core/RhythmGrammar.cpp
    src/core/PitchGrammar.cpp
    src/core/PhraseComposer.cpp
    src/core/PatternEvaluator.cpp
    src/core/PatternMetrics.cpp
    src/core/PatternGenerator.cpp
    src/core/PatternHistory.cpp
    src/core/Serialization.cpp
    src/dsp/Envelope.cpp
    src/dsp/Filter.cpp
    src/dsp/Saturation.cpp
    src/dsp/BassVoice.cpp
    src/dsp/BassSynth.cpp
    src/dsp/SynthParams.cpp
    src/dsp/NoteScheduler.cpp
    src/midi/MidiFileExport.cpp
)
target_include_directories(gfb_core PUBLIC src)
if(MSVC)
    target_compile_options(gfb_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(gfb_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion)
    # Clang contracts a*b+c into FMA by default; on arm64 (Apple Silicon) that changes
    # evaluator scores in the last bit and therefore the chosen candidate. Keep IEEE
    # separate rounding everywhere so patterns are identical on every platform.
    target_compile_options(gfb_core PRIVATE -ffp-contract=off)
endif()

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
if(GFB_BUILD_PLUGIN OR GFB_BUILD_TESTS)
    if(GFB_JUCE_DIR)
        add_subdirectory(${GFB_JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG ${GFB_JUCE_TAG}
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()
endif()

if(GFB_BUILD_PLUGIN)
    set(GFB_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND GFB_FORMATS AU)   # Logic Pro / GarageBand
    endif()
    juce_add_plugin(GrooveForgeBass
        COMPANY_NAME "GrooveForge Audio"
        COMPANY_WEBSITE ""
        BUNDLE_ID "com.grooveforge.bass"
        PLUGIN_MANUFACTURER_CODE Gfau
        PLUGIN_CODE Gfb1
        FORMATS ${GFB_FORMATS}
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        PRODUCT_NAME "GrooveForge Bass"
        DESCRIPTION "Bassline generator and bass synthesizer"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        VST3_CATEGORIES Instrument Synth Generator
        COPY_PLUGIN_AFTER_BUILD FALSE
        NEEDS_WEB_BROWSER FALSE
        NEEDS_CURL FALSE)

    target_sources(GrooveForgeBass PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/state/StateCodec.cpp
        src/state/FavoritesStore.cpp
        src/midi/MidiDragSource.cpp
        src/ui/DesignSystem.cpp
        src/ui/PianoRoll.cpp
        src/ui/SeedBox.cpp
        src/ui/GeneratorPanel.cpp
        src/ui/SynthPanel.cpp
        src/ui/BrowserPanel.cpp)

    target_compile_definitions(GrooveForgeBass PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
        GFB_VERSION_STRING="${PROJECT_VERSION}")

    target_link_libraries(GrooveForgeBass
        PRIVATE
            gfb_core
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

if(GFB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(GFB_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
