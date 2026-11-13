// Host-level integration test. Loads the *built* GrooveForge Bass .vst3 bundle
// through JUCE's VST3 hosting implementation (the same code path JUCE-based
// hosts use), then verifies: discovery as an instrument, audio rendering with
// a playing transport, MIDI output, silence when stopped, state save/restore
// into a fresh instance, multiple sample rates / block sizes, editor creation.
// This is not a DAW; it is an automated VST3 host harness.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::printf ("[%s] %s\n", ok ? " OK " : "FAIL", what);
    failures += ok ? 0 : 1;
}

struct TestPlayHead : juce::AudioPlayHead
{
    bool playing = true;
    double ppq = 0, bpm = 124;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setIsPlaying (playing);
        p.setPpqPosition (ppq);
        p.setBpm (bpm);
        p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};

struct RunResult { double rms = 0, peak = 0; bool finite = true; int noteOns = 0, noteOffs = 0; };

RunResult run (juce::AudioPluginInstance& p, TestPlayHead& ph, double sr, int block, int numBlocks)
{
    RunResult r;
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    double acc = 0;
    long n = 0;
    for (int b = 0; b < numBlocks; ++b)
    {
        midi.clear();
        buf.clear();
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < block; ++i)
            {
                const float v = buf.getSample (c, i);
                r.finite = r.finite && std::isfinite (v);
                r.peak = std::max (r.peak, (double) std::abs (v));
                acc += (double) v * v;
                ++n;
            }
        for (const auto m : midi)
        {
            r.noteOns += m.getMessage().isNoteOn() ? 1 : 0;
            r.noteOffs += m.getMessage().isNoteOff() ? 1 : 0;
        }
        if (ph.playing) ph.ppq += block * ph.bpm / 60.0 / sr;
    }
    r.rms = std::sqrt (acc / std::max (1L, n));
    return r;
}

// The hosted VST3 state is JUCE's "VST3PluginState" XML holding the base64
// IComponent stream; inside it is our own copyXmlToBinary block ("VC2!").
struct PluginChunk
{
    std::unique_ptr<juce::XmlElement> outer;
    juce::MemoryBlock component;
    size_t start = 0, length = 0;
    std::unique_ptr<juce::XmlElement> ours;
};

PluginChunk openChunk (const juce::MemoryBlock& state)
{
    PluginChunk c;
    c.outer = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
    if (c.outer == nullptr) return c;
    auto* comp = c.outer->getChildByName ("IComponent");
    if (comp == nullptr || ! c.component.fromBase64Encoding (comp->getAllSubText())) return c;
    const auto* bytes = static_cast<const char*> (c.component.getData());
    for (size_t i = 0; i + 8 <= c.component.getSize(); ++i)
        if (bytes[i] == 'V' && bytes[i + 1] == 'C' && bytes[i + 2] == '2' && bytes[i + 3] == '!')
        {
            const auto len = (size_t) juce::ByteOrder::littleEndianInt (bytes + i + 4);
            c.start = i;
            c.length = 8 + len;
            c.ours = juce::AudioProcessor::getXmlFromBinary (bytes + i, (int) juce::jmin (c.length, c.component.getSize() - i));
            break;
        }
    return c;
}

juce::MemoryBlock withStateFlag (const juce::MemoryBlock& state, const juce::String& attr, const juce::String& value)
{
    auto c = openChunk (state);
    if (c.ours == nullptr) return state;
    if (auto* gs = c.ours->getChildByName ("GrooveForgeState")) gs->setAttribute (attr, value);
    juce::MemoryBlock inner;
    juce::AudioProcessor::copyXmlToBinary (*c.ours, inner);
    juce::MemoryBlock rebuilt;
    rebuilt.append (c.component.getData(), c.start);
    rebuilt.append (inner.getData(), inner.getSize());
    const auto tail = c.start + c.length;
    if (tail < c.component.getSize()) rebuilt.append (static_cast<const char*> (c.component.getData()) + tail, c.component.getSize() - tail);
    c.outer->getChildByName ("IComponent")->deleteAllTextElements();
    c.outer->getChildByName ("IComponent")->addTextElement (rebuilt.toBase64Encoding());
    juce::MemoryBlock out;
    juce::AudioProcessor::copyXmlToBinary (*c.outer, out);
    return out;
}

juce::String stateAttr (const juce::MemoryBlock& state, const juce::String& attr)
{
    auto c = openChunk (state);
    if (c.ours == nullptr) return "<no plugin state>";
    if (auto* gs = c.ours->getChildByName ("GrooveForgeState")) return gs->getStringAttribute (attr, "<missing>");
    return "<no GrooveForgeState>";
}
} // namespace

int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: gfb_host_test <path to GrooveForge Bass.vst3> [--snapshot out.png width height scale]\n"); return 2; }
    const bool snapshotMode = argc >= 7 && juce::String (argv[2]) == "--snapshot";
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File bundle { juce::String (juce::CharPointer_UTF8 (argv[1])) };
    check (bundle.exists(), "VST3 bundle exists");

    juce::VST3PluginFormat vst3;
    juce::OwnedArray<juce::PluginDescription> descs;
    vst3.findAllTypesForFile (descs, bundle.getFullPathName());
    check (descs.size() == 1, "exactly one plugin type discovered in bundle");
    if (descs.isEmpty()) return 1;
    const auto& d = *descs[0];
    std::printf ("    name='%s' vendor='%s' category='%s' instrument=%d version=%s uid=%08x\n", d.name.toRawUTF8(), d.manufacturerName.toRawUTF8(),
                 d.category.toRawUTF8(), (int) d.isInstrument, d.version.toRawUTF8(), (unsigned) d.uniqueId);
    check (d.name == "GrooveForge Bass", "plugin name");
    check (d.isInstrument, "registered as instrument");

    juce::String err;
    auto inst = vst3.createInstanceFromDescription (d, 48000.0, 512, err);
    check (inst != nullptr, ("instance created " + err).toRawUTF8());
    if (inst == nullptr) return 1;
    check (inst->getParameters().size() >= 10, "at least 10 host parameters (8 macros + shape + engine)");
    for (auto* prm : inst->getParameters()) std::printf ("    param: %s = %s\n", prm->getName (32).toRawUTF8(), prm->getCurrentValueAsText().toRawUTF8());
    check (inst->getLatencySamples() >= 0, "latency reported");

    TestPlayHead ph;
    inst->setPlayHead (&ph);
    if (snapshotMode)
    {
        // Renders the real editor (as hosted) to a PNG for visual QA.
        inst->prepareToPlay (48000.0, 512);
        run (*inst, ph, 48000.0, 512, 100);
        std::unique_ptr<juce::AudioProcessorEditor> ed (inst->createEditorAndMakeActive());
        const int w = juce::String (argv[4]).getIntValue(), h = juce::String (argv[5]).getIntValue();
        const float scale = (float) juce::String (argv[6]).getDoubleValue();
        ed->setSize (w, h);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (400);
        const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        juce::File out { juce::String (argv[3]) };
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat png;
        const bool ok = os.openedOk() && png.writeImageToStream (img, os);
        std::printf ("snapshot %s: %dx%d @%.2f\n", ok ? "written" : "FAILED", img.getWidth(), img.getHeight(), scale);
        ed.reset();
        return ok ? 0 : 1;
    }
    inst->enableAllBuses();
    inst->prepareToPlay (48000.0, 512);
    auto r = run (*inst, ph, 48000.0, 512, 600); // ~6.4 s at 124 BPM
    std::printf ("    playing: rms %.4f peak %.4f\n", r.rms, r.peak);
    check (r.finite, "audio finite while playing");
    check (r.rms > 0.01, "audible output while host transport plays");
    check (r.peak < 1.0, "no clipping with default settings");

    // stop: must fall silent (no stuck notes)
    ph.playing = false;
    run (*inst, ph, 48000.0, 512, 40);
    auto s = run (*inst, ph, 48000.0, 512, 40);
    check (s.peak < 1.0e-4, "silent after transport stop (no stuck note)");

    // MIDI out via state flag
    juce::MemoryBlock st;
    inst->getStateInformation (st);
    check (st.getSize() > 100, "state chunk produced");
    const auto pattern = stateAttr (st, "pattern");
    check (pattern.contains ("|"), "state contains exact pattern snapshot");
    const auto midiOn = withStateFlag (st, "midiOut", "1");
    check (stateAttr (midiOn, "midiOut") == "1", "state chunk editable by harness");
    inst->setStateInformation (midiOn.getData(), (int) midiOn.getSize());
    ph.playing = true;
    ph.ppq = 0;
    auto m = run (*inst, ph, 48000.0, 512, 600);
    std::printf ("    midi out: %d note-on, %d note-off\n", m.noteOns, m.noteOffs);
    check (m.noteOns > 4, "VST3 MIDI output carries generated notes (harness host)");
    check (std::abs (m.noteOns - m.noteOffs) <= 2, "MIDI out note-ons balanced by note-offs");

    // restore into a fresh instance: exact pattern + settings
    juce::MemoryBlock st2;
    inst->getStateInformation (st2);
    auto inst2 = vst3.createInstanceFromDescription (d, 48000.0, 512, err);
    check (inst2 != nullptr, "second instance");
    if (inst2 != nullptr)
    {
        inst2->setStateInformation (st2.getData(), (int) st2.getSize());
        juce::MemoryBlock st3;
        inst2->getStateInformation (st3);
        check (stateAttr (st2, "pattern").contains ("|") && stateAttr (st3, "pattern") == stateAttr (st2, "pattern"), "project reload restores the exact pattern");
        check (stateAttr (st3, "midiOut") == "1", "project reload restores MIDI-out option");
        check (stateAttr (st3, "settings") == stateAttr (st2, "settings"), "project reload restores generator settings");
        check (stateAttr (st3, "locks") == stateAttr (st2, "locks"), "project reload restores locks");
        bool paramsEqual = true;
        for (int i = 0; i < inst->getParameters().size(); ++i)
            paramsEqual = paramsEqual && std::abs (inst->getParameters()[i]->getValue() - inst2->getParameters()[i]->getValue()) < 1e-6f;
        check (paramsEqual, "project reload restores synth parameters");
        // corrupted chunk must not crash or change state
        const char junk[] = "this is not a valid state";
        inst2->setStateInformation (junk, (int) sizeof (junk));
        juce::MemoryBlock st4;
        inst2->getStateInformation (st4);
        check (stateAttr (st4, "pattern") == stateAttr (st2, "pattern"), "corrupted state chunk ignored safely");
    }

    // sample rates and block sizes
    bool allFinite = true, allAudible = true;
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int bs : { 32, 64, 128, 256, 512, 1024 })
        {
            inst->releaseResources();
            inst->prepareToPlay (sr, bs);
            ph.playing = true;
            ph.ppq = 0;
            const auto rr = run (*inst, ph, sr, bs, (int) (sr * 2.5 / bs));
            allFinite = allFinite && rr.finite && rr.peak < 1.5;
            allAudible = allAudible && rr.rms > 0.005;
        }
    check (allFinite, "finite & bounded at 44.1/48/96 kHz x 32..1024 samples");
    check (allAudible, "audible at all tested rates/block sizes");

    // editor lifecycle while "transport running"
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (inst->createEditorAndMakeActive());
        check (ed != nullptr, "editor created");
        if (ed)
        {
            ed->setSize (1300, 820);
            run (*inst, ph, 96000.0, 1024, 20);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
        }
        ed.reset();
        run (*inst, ph, 96000.0, 1024, 20);
        check (true, "editor destroyed while processing");
    }
    inst->releaseResources();
    inst.reset();
    std::printf ("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
