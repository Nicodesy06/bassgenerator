# ---------------------------------------------------------------------------
# Pure C++ tests (no JUCE, no audio device)
# ---------------------------------------------------------------------------
add_executable(gfb_tests
    TestMain.cpp
    TheoryTests.cpp
    GeneratorTests.cpp
    PatternMetricsTests.cpp
    MidiExportTests.cpp
    SchedulerTests.cpp
    DspTests.cpp
    RealtimeTests.cpp
    TasteTests.cpp)
target_link_libraries(gfb_tests PRIVATE gfb_core)
target_include_directories(gfb_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

foreach(suite Theory Seed Corpus Rhythm Settings Generator History Serialization Metrics Midi Scheduler Dsp Realtime Taste)
    add_test(NAME ${suite} COMMAND gfb_tests "${suite}.")
    set_tests_properties(${suite} PROPERTIES TIMEOUT 900)
endforeach()

# ---------------------------------------------------------------------------
# JUCE-based tests: state codec, favorites store, juce::MidiFile cross-check
# ---------------------------------------------------------------------------
juce_add_console_app(gfb_juce_tests PRODUCT_NAME "gfb_juce_tests")
target_sources(gfb_juce_tests PRIVATE
    TestMain.cpp
    PersistenceTests.cpp
    JuceMidiCrossCheckTests.cpp
    ${PROJECT_SOURCE_DIR}/src/state/StateCodec.cpp
    ${PROJECT_SOURCE_DIR}/src/state/FavoritesStore.cpp)
target_include_directories(gfb_juce_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(gfb_juce_tests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(gfb_juce_tests PRIVATE gfb_core juce::juce_core juce::juce_data_structures juce::juce_audio_basics
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
foreach(suite State Favorites JuceMidi)
    add_test(NAME ${suite} COMMAND gfb_juce_tests "${suite}.")
    set_tests_properties(${suite} PROPERTIES TIMEOUT 300)
endforeach()

# ---------------------------------------------------------------------------
# Host-level integration test: loads the built VST3 bundle through JUCE's
# VST3 hosting code, renders audio, checks MIDI out and state round trip.
# ---------------------------------------------------------------------------
if(GFB_BUILD_PLUGIN)
    juce_add_console_app(gfb_host_test PRODUCT_NAME "gfb_host_test")
    target_sources(gfb_host_test PRIVATE HostIntegrationTest.cpp)
    target_compile_definitions(gfb_host_test PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
    target_link_libraries(gfb_host_test PRIVATE juce::juce_audio_processors juce::juce_audio_formats
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_dependencies(gfb_host_test GrooveForgeBass_VST3)
    add_test(NAME HostIntegration COMMAND gfb_host_test "${PROJECT_BINARY_DIR}/GrooveForgeBass_artefacts/$<CONFIG>/VST3/GrooveForge Bass.vst3")
    set_tests_properties(HostIntegration PROPERTIES TIMEOUT 300)
endif()

# ---------------------------------------------------------------------------
# In-process GUI binding tests (real editor + real processor, no host)
# ---------------------------------------------------------------------------
if(GFB_BUILD_PLUGIN)
    juce_add_console_app(gfb_ui_tests PRODUCT_NAME "gfb_ui_tests")
    target_sources(gfb_ui_tests PRIVATE UiTests.cpp)
    target_include_directories(gfb_ui_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(gfb_ui_tests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
    target_link_libraries(gfb_ui_tests PRIVATE GrooveForgeBass gfb_core juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME UiBindings COMMAND gfb_ui_tests)
    set_tests_properties(UiBindings PROPERTIES TIMEOUT 300)
endif()
