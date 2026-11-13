#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "dsp/SynthParams.h"
#include "midi/MidiDragSource.h"

namespace gfb
{
// ============================================================================ exchange
void ScheduleExchange::publish (const Pattern& p, uint64_t version)
{
    std::lock_guard<std::mutex> g (writerMutex);
    buffers[(size_t) writeIndex].fromPattern (p, version);
    writeIndex = middle.exchange (writeIndex | kDirty) & 3;
}

const NoteSchedule& ScheduleExchange::acquire() noexcept
{
    if (middle.load (std::memory_order_acquire) & kDirty)
        readIndex = middle.exchange (readIndex, std::memory_order_acq_rel) & 3;
    return buffers[(size_t) readIndex];
}

// ============================================================================ params
namespace
{
const juce::ParameterID idVolume { "volume", 1 }, idCutoff { "cutoff", 1 }, idRes { "resonance", 1 }, idEnv { "envmod", 1 },
    idDecay { "decay", 1 }, idAccent { "accent", 1 }, idDrive { "drive", 1 }, idGlide { "glide", 1 }, idShape { "shape", 1 },
    idEngine { "engine", 1 };

juce::String pct (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }

uint64_t freshSeed()
{
    return (uint64_t) juce::Random::getSystemRandom().nextInt (juce::Range<int> (1, 1000000));
}

const char* kParamIds[] = { "volume", "cutoff", "resonance", "envmod", "decay", "accent", "drive", "glide", "shape", "engine" };
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout GrooveForgeProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using A = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    v.push_back (std::make_unique<P> (idVolume, "Volume", juce::NormalisableRange<float> (-60.0f, 6.0f, 0.1f, 2.5f), -6.0f,
                                      A().withLabel ("dB").withStringFromValueFunction ([] (float x, int) { return x <= -59.9f ? juce::String ("-inf dB") : juce::String (x, 1) + " dB"; })));
    v.push_back (std::make_unique<P> (idCutoff, "Cutoff", juce::NormalisableRange<float> (0.0f, 1.0f), 0.48f,
                                      A().withStringFromValueFunction ([] (float x, int) { const float hz = synthmap::cutoffHz (x); return hz >= 1000 ? juce::String (hz / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz"; })));
    v.push_back (std::make_unique<P> (idRes, "Resonance", juce::NormalisableRange<float> (0.0f, 1.0f), 0.30f, A().withStringFromValueFunction (pct)));
    v.push_back (std::make_unique<P> (idEnv, "Env Mod", juce::NormalisableRange<float> (0.0f, 1.0f), 0.45f, A().withStringFromValueFunction (pct)));
    v.push_back (std::make_unique<P> (idDecay, "Decay", juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f,
                                      A().withStringFromValueFunction ([] (float x, int) { const float s = synthmap::decaySeconds (x); return s >= 1 ? juce::String (s, 2) + " s" : juce::String (juce::roundToInt (s * 1000)) + " ms"; })));
    v.push_back (std::make_unique<P> (idAccent, "Accent", juce::NormalisableRange<float> (0.0f, 1.0f), 0.55f, A().withStringFromValueFunction (pct)));
    v.push_back (std::make_unique<P> (idDrive, "Drive", juce::NormalisableRange<float> (0.0f, 1.0f), 0.20f, A().withStringFromValueFunction (pct)));
    v.push_back (std::make_unique<P> (idGlide, "Glide", juce::NormalisableRange<float> (0.0f, 1.0f), 0.45f,
                                      A().withStringFromValueFunction ([] (float x, int) { return juce::String (juce::roundToInt (synthmap::glideSeconds (x) * 1000)) + " ms"; })));
    v.push_back (std::make_unique<P> (idShape, "Shape", juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f, A().withStringFromValueFunction (pct)));
    v.push_back (std::make_unique<juce::AudioParameterChoice> (idEngine, "Engine", juce::StringArray { "SUB", "ACID" }, 0));
    return { v.begin(), v.end() };
}

// ============================================================================ lifecycle
GrooveForgeProcessor::GrooveForgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pVolume = apvts.getRawParameterValue ("volume");
    pCutoff = apvts.getRawParameterValue ("cutoff");
    pRes = apvts.getRawParameterValue ("resonance");
    pEnv = apvts.getRawParameterValue ("envmod");
    pDecay = apvts.getRawParameterValue ("decay");
    pAccent = apvts.getRawParameterValue ("accent");
    pDrive = apvts.getRawParameterValue ("drive");
    pGlide = apvts.getRawParameterValue ("glide");
    pShape = apvts.getRawParameterValue ("shape");
    pEngine = apvts.getRawParameterValue ("engine");

    favoritesStore = std::make_unique<FavoritesStore>();
    mididrag::cleanupDragCache (24.0);

    // cold start: reproducible but randomly initialised seeds + a valid pattern
    std::lock_guard<std::mutex> g (stateMutex);
    st.settings.styleSeed = freshSeed();
    st.settings.varSeed = freshSeed();
    setPatternLocked (generatePattern (st.settings), true);
}

GrooveForgeProcessor::~GrooveForgeProcessor() { cancelPendingUpdate(); }

bool GrooveForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void GrooveForgeProcessor::prepareToPlay (double sr, int block)
{
    sampleRate = sr;
    scheduler.prepare (sr);
    synth.prepare (sr, block);
    previewPpq = 0.0;
    setLatencySamples (15); // 2x oversampled drive FIR (up + down half-band, 31 taps each)
}

// Hosts / the VST3 wrapper may call these from the audio thread: no locks there,
// and preset application (which notifies the host of parameter changes) is
// always performed on the message thread.
int GrooveForgeProcessor::getCurrentProgram() { return currentProgram.load(); }
void GrooveForgeProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    // Never apply synchronously: the VST3 wrapper can call this from inside
    // process() (program-change parameter), and pushing 10 parameter edits back
    // to the host from there is re-entrant. The index is visible immediately.
    currentProgram = index;
    pendingProgram = index;
    triggerAsyncUpdate();
}
void GrooveForgeProcessor::handleAsyncUpdate()
{
    const int p = pendingProgram.exchange (-1);
    if (p >= 0) applyPreset (p);
}
const juce::String GrooveForgeProcessor::getProgramName (int index)
{
    const auto p = factoryPresets();
    return index >= 0 && index < (int) p.size() ? juce::String (std::string (p[(size_t) index].name)) : juce::String();
}

// ============================================================================ audio
void GrooveForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;

    SynthParams sp;
    sp.volumeDb = pVolume->load();
    sp.cutoff = pCutoff->load();
    sp.resonance = pRes->load();
    sp.envMod = pEnv->load();
    sp.decay = pDecay->load();
    sp.accent = pAccent->load();
    sp.drive = pDrive->load();
    sp.glide = pGlide->load();
    sp.shape = pShape->load();
    sp.engine = pEngine->load() > 0.5f ? SynthEngine::Acid : SynthEngine::Sub;

    const NoteSchedule& sched = exchange.acquire();

    // ---- transport policy -------------------------------------------------
    bool hostPlaying = false, hostHasPos = false;
    double hostPpq = 0.0, hostBpm = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            hostPlaying = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) { hostPpq = *p; hostHasPos = true; }
            if (auto bpm = pos->getBpm()) hostBpm = *bpm;
        }
    const double fallbackBpm = previewBpm.load();
    const double bpm = hostBpm > 0.0 ? hostBpm : fallbackBpm;
    TransportState t;
    if (hostSyncOn.load() && hostPlaying && hostHasPos)
        t = { true, hostPpq, bpm };
    else if (previewOn.load())
    {
        if (previewRestart.exchange (false)) previewPpq = 0.0;
        t = { true, previewPpq, bpm };
        previewPpq += n * bpm / 60.0 / sampleRate;
        const double loopQ = (double) sched.lengthTicks / kPPQ;
        if (previewPpq > loopQ * 1024.0) previewPpq = std::fmod (previewPpq, loopQ); // keep precision on very long previews
    }
    else
        t.playing = false;

    int ne = scheduler.process (sched, t, n, events.data(), (int) events.size());

    // ---- merge live MIDI input (play the synth from a keyboard / clip) ------
    int nm = 0, ie = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        SynthEvent e;
        e.sampleOffset = juce::jlimit (0, n - 1, meta.samplePosition);
        if (msg.isNoteOn()) { e.type = SynthEvent::Type::NoteOn; e.note = (uint8_t) msg.getNoteNumber(); e.velocity = msg.getVelocity(); e.accent = msg.getVelocity() >= 110; }
        else if (msg.isNoteOff()) { e.type = SynthEvent::Type::NoteOff; e.note = (uint8_t) msg.getNoteNumber(); }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { e.type = SynthEvent::Type::AllNotesOff; }
        else continue;
        while (ie < ne && events[(size_t) ie].sampleOffset <= e.sampleOffset && nm < (int) merged.size()) merged[(size_t) nm++] = events[(size_t) ie++];
        if (nm < (int) merged.size()) merged[(size_t) nm++] = e;
    }
    while (ie < ne && nm < (int) merged.size()) merged[(size_t) nm++] = events[(size_t) ie++];

    // ---- render ---------------------------------------------------------------
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    synth.process (L, R, n, merged.data(), nm, sp);
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);

    // ---- MIDI out: the generated line, same events as the internal synth -----
    midi.clear();
    if (midiOutOn.load())
    {
        for (int i = 0; i < ne; ++i)
        {
            const auto& e = events[(size_t) i];
            if (e.type == SynthEvent::Type::NoteOn) midi.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.velocity), e.sampleOffset);
            else if (e.type == SynthEvent::Type::NoteOff) midi.addEvent (juce::MidiMessage::noteOff (1, e.note, (juce::uint8) 64), e.sampleOffset);
        }
    }
    generatedEventsTotal.fetch_add (ne, std::memory_order_relaxed);

    // ---- UI feedback ----------------------------------------------------------
    float peak = 0.0f;
    const auto* out0 = buffer.getReadPointer (0);
    for (int i = 0; i < n; ++i) peak = juce::jmax (peak, (float) std::abs (out0[i]));
    uiPeak.store (juce::jmax (peak, uiPeak.load() * 0.85f));
    uiHostPlaying.store (hostPlaying);
    uiHostHasPosition.store (hostHasPos);
    uiHostBpm.store (hostBpm);
    uiPlayheadTick.store (t.playing ? std::fmod (std::fmod (t.ppqPosition * kPPQ, (double) sched.lengthTicks) + (double) sched.lengthTicks, (double) sched.lengthTicks) : -1.0);
}

// ============================================================================ state API
void GrooveForgeProcessor::publishLocked()
{
    exchange.publish (st.pattern, ++scheduleVersion);
}

void GrooveForgeProcessor::setPatternLocked (const Pattern& p, bool pushHistory)
{
    st.pattern = p;
    st.settings = p.settings;
    if (pushHistory) history.push (p);
    st.historyCursor = history.cursor();
    publishLocked();
}

state::InstanceState GrooveForgeProcessor::getStateSnapshot() const
{
    std::lock_guard<std::mutex> g (stateMutex);
    auto s = st;
    s.history = history.all();
    s.historyCursor = history.cursor();
    s.hostSync = hostSyncOn.load();
    s.midiOut = midiOutOn.load();
    s.previewBpm = previewBpm.load();
    return s;
}

Pattern GrooveForgeProcessor::getPattern() const
{
    std::lock_guard<std::mutex> g (stateMutex);
    return st.pattern;
}

void GrooveForgeProcessor::generate (RollMode mode)
{
    bool familyChanged;
    Family fam;
    {
        std::lock_guard<std::mutex> g (stateMutex);
        const auto s = rollSettings (st.settings, st.locks, mode, (uint64_t) juce::Random::getSystemRandom().nextInt64());
        familyChanged = s.family != st.settings.family;
        fam = s.family;
        setPatternLocked (generatePattern (s), true);
    }
    if (familyChanged) applyPreset (fam == Family::Acid ? 4 : 1);
    bump();
}

void GrooveForgeProcessor::setSettings (const GeneratorSettings& in)
{
    bool familyChanged;
    Family fam;
    {
        std::lock_guard<std::mutex> g (stateMutex);
        const auto s = sanitise (in);
        familyChanged = s.family != st.settings.family;
        fam = s.family;
        setPatternLocked (generatePattern (s), true);
    }
    if (familyChanged) applyPreset (fam == Family::Acid ? 4 : 1); // engine follows the family; user may pick any preset after
    bump();
}

void GrooveForgeProcessor::setLocks (const LockSet& l)
{
    { std::lock_guard<std::mutex> g (stateMutex); st.locks = l; }
    bump();
}

bool GrooveForgeProcessor::historyBack()
{
    std::lock_guard<std::mutex> g (stateMutex);
    auto p = history.back();
    if (! p) return false;
    setPatternLocked (*p, false);
    bump();
    return true;
}

bool GrooveForgeProcessor::historyForward()
{
    std::lock_guard<std::mutex> g (stateMutex);
    auto p = history.forward();
    if (! p) return false;
    setPatternLocked (*p, false);
    bump();
    return true;
}

bool GrooveForgeProcessor::historySelect (int index)
{
    std::lock_guard<std::mutex> g (stateMutex);
    auto p = history.select (index);
    if (! p) return false;
    setPatternLocked (*p, false);
    bump();
    return true;
}

void GrooveForgeProcessor::loadPattern (const Pattern& p)
{
    if (! validatePattern (p).empty()) return;
    { std::lock_guard<std::mutex> g (stateMutex); setPatternLocked (p, true); }
    bump();
}

void GrooveForgeProcessor::applyPreset (int index)
{
    const auto presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    const auto& p = presets[(size_t) index].params;
    auto set = [this] (const char* id, float plain) {
        if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
    };
    set ("volume", p.volumeDb); set ("cutoff", p.cutoff); set ("resonance", p.resonance); set ("envmod", p.envMod); set ("decay", p.decay);
    set ("accent", p.accent); set ("drive", p.drive); set ("glide", p.glide); set ("shape", p.shape);
    set ("engine", p.engine == SynthEngine::Acid ? 1.0f : 0.0f);
    { std::lock_guard<std::mutex> g (stateMutex); st.presetIndex = index; }
    currentProgram = index;
    bump();
}

void GrooveForgeProcessor::setHostSync (bool on) { hostSyncOn = on; bump(); }
void GrooveForgeProcessor::setMidiOut (bool on) { midiOutOn = on; bump(); }
void GrooveForgeProcessor::setPreview (bool on)
{
    if (on && ! previewOn.load()) previewRestart = true;
    previewOn = on;
    bump();
}
void GrooveForgeProcessor::setPreviewBpm (double bpm) { previewBpm = juce::jlimit (40.0, 240.0, bpm); bump(); }
void GrooveForgeProcessor::setEditorSize (int w, int h) { std::lock_guard<std::mutex> g (stateMutex); st.editorWidth = w; st.editorHeight = h; }
void GrooveForgeProcessor::setDrawerOpen (bool open) { std::lock_guard<std::mutex> g (stateMutex); st.drawerOpen = open; }

void GrooveForgeProcessor::resetToDefaults()
{
    {
        std::lock_guard<std::mutex> g (stateMutex);
        GeneratorSettings s;
        s.styleSeed = st.settings.styleSeed;
        s.varSeed = st.settings.varSeed;
        st.locks = LockSet {};
        setPatternLocked (generatePattern (s), true);
    }
    for (auto* id : kParamIds)
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->getDefaultValue());
    { std::lock_guard<std::mutex> g (stateMutex); st.presetIndex = 1; }
    currentProgram = 1;
    hostSyncOn = true;
    midiOutOn = false;
    bump();
}

double GrooveForgeProcessor::exportBpm() const
{
    const double h = uiHostBpm.load();
    return h > 0.0 ? h : previewBpm.load();
}

juce::Result GrooveForgeProcessor::exportMidi (const juce::File& target)
{
    return mididrag::exportTo (getPattern(), exportBpm(), target);
}

juce::Result GrooveForgeProcessor::prepareDragFile (juce::File& out)
{
    return mididrag::writeDragFile (getPattern(), exportBpm(), out);
}

// ============================================================================ persistence
void GrooveForgeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ValueTree root ("GrooveForgeBass");
    root.setProperty ("version", GFB_VERSION_STRING, nullptr);
    root.appendChild (apvts.copyState(), nullptr);
    root.appendChild (state::encode (getStateSnapshot()), nullptr);
    if (auto xml = root.createXml()) copyXmlToBinary (*xml, dest);
}

void GrooveForgeProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr) return; // corrupt chunk: keep current state
    const auto root = juce::ValueTree::fromXml (*xml);
    if (! root.hasType ("GrooveForgeBass")) return;
    const auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid()) apvts.replaceState (params);
    if (auto s = state::decode (root.getChildWithName (state::kRootId)))
    {
        std::lock_guard<std::mutex> g (stateMutex);
        const auto pattern = s->pattern;
        st = *s;
        history.assign (s->history, s->historyCursor);
        // exact snapshot restore; regenerate only if the stored notes were unusable
        if (pattern.notes.empty() || ! validatePattern (pattern).empty())
            st.pattern = generatePattern (st.settings);
        else
            st.pattern.settings = pattern.settings;
        st.settings = st.pattern.settings;
        currentProgram = s->presetIndex;
        hostSyncOn = s->hostSync;
        midiOutOn = s->midiOut;
        previewBpm = s->previewBpm;
        publishLocked();
    }
    bump();
}

juce::AudioProcessorEditor* GrooveForgeProcessor::createEditor() { return new GrooveForgeEditor (*this); }
} // namespace gfb

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new gfb::GrooveForgeProcessor(); }
