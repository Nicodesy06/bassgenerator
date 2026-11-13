#pragma once
#include <array>
#include <atomic>
#include <mutex>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/PatternGenerator.h"
#include "core/PatternHistory.h"
#include "dsp/BassSynth.h"
#include "dsp/NoteScheduler.h"
#include "state/FavoritesStore.h"
#include "state/StateCodec.h"

namespace gfb
{
/** Single-writer / single-reader triple buffer for the note schedule.
    publish() (message/background thread, serialised by a mutex) never
    blocks the reader; acquire() (audio thread) is wait-free and allocation
    free. Buffers are preallocated members, so nothing is ever freed on the
    audio thread. */
class ScheduleExchange
{
public:
    void publish (const Pattern& p, uint64_t version);
    const NoteSchedule& acquire() noexcept;

private:
    static constexpr int kDirty = 4;
    std::array<NoteSchedule, 3> buffers;
    std::atomic<int> middle { 1 };
    int writeIndex = 0, readIndex = 2;
    std::mutex writerMutex;
};

class GrooveForgeProcessor final : public juce::AudioProcessor
{
public:
    GrooveForgeProcessor();
    ~GrooveForgeProcessor() override;

    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    bool supportsDoublePrecisionProcessing() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "GrooveForge Bass"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 8; }
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- message-thread API (thread-safe, never called from audio) ----------
    state::InstanceState getStateSnapshot() const;
    Pattern getPattern() const;
    void generate (RollMode mode);
    /** Changes generator settings; regenerates with the current seeds. */
    void setSettings (const GeneratorSettings& s);
    void setLocks (const LockSet& l);
    bool historyBack();
    bool historyForward();
    bool historySelect (int index);
    /** Restore an exact pattern (favorites): notes and generator settings. */
    void loadPattern (const Pattern& p);
    void applyPreset (int index);
    void setHostSync (bool on);
    void setMidiOut (bool on);
    void setPreview (bool on);
    bool isPreviewOn() const noexcept { return previewOn.load(); }
    void setPreviewBpm (double bpm);
    void setEditorSize (int w, int h);
    void setDrawerOpen (bool open);
    void resetToDefaults();
    double exportBpm() const;
    juce::Result exportMidi (const juce::File& target);
    juce::Result prepareDragFile (juce::File& out);
    FavoritesStore& favorites() { return *favoritesStore; }

    /** Increments whenever generator state changes (UI polls it). */
    std::atomic<uint32_t> stateVersion { 1 };

    // realtime info for the UI (written by the audio thread)
    std::atomic<double> uiPlayheadTick { -1.0 };
    std::atomic<double> uiHostBpm { 0.0 };
    std::atomic<bool> uiHostPlaying { false };
    std::atomic<bool> uiHostHasPosition { false };
    std::atomic<float> uiPeak { 0.0f };

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<int> generatedEventsTotal { 0 }; // diagnostics/tests

private:
    void setPatternLocked (const Pattern& p, bool pushHistory);
    void publishLocked();
    void bump() { stateVersion.fetch_add (1); }

    mutable std::mutex stateMutex;
    state::InstanceState st;
    PatternHistory history;
    uint64_t scheduleVersion = 1;
    ScheduleExchange exchange;

    NoteScheduler scheduler;
    dsp::BassSynth synth;
    std::atomic<bool> previewOn { false }, hostSyncOn { true }, midiOutOn { false }, previewRestart { false };
    std::atomic<double> previewBpm { 124.0 };
    double previewPpq = 0.0, sampleRate = 44100.0;
    std::array<SynthEvent, 1024> events {};
    std::array<SynthEvent, 1024> merged {};

    std::atomic<float>* pVolume = nullptr; std::atomic<float>* pCutoff = nullptr; std::atomic<float>* pRes = nullptr;
    std::atomic<float>* pEnv = nullptr; std::atomic<float>* pDecay = nullptr; std::atomic<float>* pAccent = nullptr;
    std::atomic<float>* pDrive = nullptr; std::atomic<float>* pGlide = nullptr; std::atomic<float>* pShape = nullptr;
    std::atomic<float>* pEngine = nullptr;

    std::unique_ptr<FavoritesStore> favoritesStore;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveForgeProcessor)
};
} // namespace gfb
