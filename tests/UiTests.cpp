// In-process GUI tests: the real GrooveForgeEditor bound to a real processor.
// Verifies that controls change engine state (not just visuals), exercises
// resize limits / scale factors, and writes PNG snapshots for visual QA.
//   gfb_ui_tests [snapshot-dir]
#include <cstdio>

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginEditor.h"
#include "PluginProcessor.h"

using namespace gfb;

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::printf ("[%s] %s\n", ok ? " OK " : "FAIL", what);
    failures += ok ? 0 : 1;
}

template <typename T>
T* findChild (juce::Component& root, std::function<bool (T&)> pred)
{
    for (auto* c : root.getChildren())
    {
        if (auto* t = dynamic_cast<T*> (c))
            if (pred (*t)) return t;
        if (auto* deeper = findChild<T> (*c, pred)) return deeper;
    }
    return nullptr;
}

juce::TextButton* button (juce::Component& root, const juce::String& text)
{
    return findChild<juce::TextButton> (root, [&] (juce::TextButton& b) { return b.getButtonText() == text; });
}

void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

void snapshot (juce::Component& c, const juce::File& dir, const juce::String& name, float scale = 1.0f)
{
    if (dir == juce::File()) return;
    const auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    auto f = dir.getChildFile (name + ".png");
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    png.writeImageToStream (img, os);
    std::printf ("    snapshot %s (%dx%d)\n", f.getFileName().toRawUTF8(), img.getWidth(), img.getHeight());
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File snapDir;
    if (argc > 1) { snapDir = juce::File (juce::String (argv[1])); snapDir.createDirectory(); }
    {
        GrooveForgeProcessor proc;
        proc.prepareToPlay (48000, 256);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditorAndMakeActive());
        check (ed != nullptr, "editor created");
        ed->setSize (1180, 760);
        pump (200);

        const auto before = proc.getStateSnapshot();
        auto* gen = button (*ed, "GENERATE");
        check (gen != nullptr, "GENERATE button present");
        gen->triggerClick();
        pump (100);
        const auto after = proc.getStateSnapshot();
        check (after.pattern.hash != before.pattern.hash, "GENERATE changes the engine pattern");
        check (after.settings.styleSeed != before.settings.styleSeed && after.settings.varSeed != before.settings.varSeed, "GENERATE rolls both seeds");
        check (after.history.size() == before.history.size() + 1, "GENERATE adds a history snapshot");

        // NEW VAR keeps STYLE
        button (*ed, "NEW VAR")->triggerClick();
        pump (50);
        const auto v = proc.getStateSnapshot();
        check (v.settings.styleSeed == after.settings.styleSeed && v.settings.varSeed != after.settings.varSeed, "NEW VAR changes only VAR");

        // history back restores the previous exact pattern
        button (*ed, "<")->triggerClick();
        pump (50);
        check (proc.getPattern().notes == after.pattern.notes, "history back restores the exact previous pattern");
        button (*ed, ">")->triggerClick();
        pump (50);
        check (proc.getPattern().notes == v.pattern.notes, "history forward returns");

        // family segmented -> ACID changes generator family AND synth engine
        auto* fam = findChild<ui::Segmented> (*ed, [] (ui::Segmented& s) { return s.getTooltip().startsWith ("Family"); });
        check (fam != nullptr, "family control present");
        if (fam) fam->setSelected (1, true);
        pump (100);
        check (proc.getStateSnapshot().settings.family == Family::Acid, "family control sets ACID generator family");
        check (proc.apvts.getRawParameterValue ("engine")->load() > 0.5f, "family control switches synth engine to ACID");

        // lock prevents RANDOMISE changing the root
        auto locks = proc.getStateSnapshot().locks;
        locks.root = true;
        proc.setLocks (locks);
        const int rootBefore = proc.getStateSnapshot().settings.root;
        for (int i = 0; i < 10; ++i) { button (*ed, "RANDOMISE")->triggerClick(); pump (10); }
        check (proc.getStateSnapshot().settings.root == rootBefore, "locked root survives 10 RANDOMISE clicks");

        // synth knob -> parameter
        auto* cutoff = findChild<juce::Slider> (*ed, [] (juce::Slider& s) { return s.getTooltip().startsWith ("Filter cutoff"); });
        check (cutoff != nullptr, "cutoff knob present");
        if (cutoff) cutoff->setValue (0.9, juce::sendNotificationSync);
        pump (50);
        check (std::abs (proc.apvts.getRawParameterValue ("cutoff")->load() - 0.9f) < 1e-4f, "cutoff knob drives the DSP parameter");

        // toggles
        button (*ed, "MIDI OUT")->triggerClick();
        pump (20);
        check (proc.getStateSnapshot().midiOut, "MIDI OUT toggle sets processor option");
        button (*ed, "PREVIEW")->triggerClick();
        pump (20);
        check (proc.isPreviewOn(), "PREVIEW toggle starts preview");
        // preview actually renders audio with the stopped (absent) host transport
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi;
        double acc = 0;
        for (int i = 0; i < 400; ++i) { proc.processBlock (buf, midi); acc += buf.getRMSLevel (0, 0, 256); }
        check (acc / 400 > 0.005, "preview mode produces audio without host transport");
        button (*ed, "PREVIEW")->triggerClick();

        // favorites (uses a temp copy: do not touch the user's real favorites)
        // (FavoritesStore tests cover the file logic; here we only check the button path exists)
        check (button (*ed, "+ FAV") != nullptr, "favorite button present");

        // library drawer
        button (*ed, "LIBRARY")->triggerClick();
        pump (100);
        check (proc.getStateSnapshot().drawerOpen, "LIBRARY opens the drawer (state persisted)");
        snapshot (*ed, snapDir, "editor_acid_drawer_1180x760");

        // resize limits and scale factors
        for (auto [w, h] : { std::pair { 1100, 640 }, std::pair { 1600, 1000 }, std::pair { 2560, 1600 } })
        {
            ed->setSize (w, h);
            pump (50);
            check (ed->getWidth() == w && ed->getHeight() == h, ("editor resizes to " + juce::String (w) + "x" + juce::String (h)).toRawUTF8());
        }
        if (auto* c = ed->getConstrainer())
        {
            juce::Rectangle<int> b (0, 0, 400, 300);
            c->checkBounds (b, ed->getBounds(), juce::Desktop::getInstance().getDisplays().getTotalBounds (true), false, false, false, false);
            check (b.getWidth() >= 1100 && b.getHeight() >= 640 && c->getMaximumWidth() == 2600, "resize limits enforced by the editor constrainer");
        }
        else check (false, "editor has a constrainer");
        ed->setSize (1100, 640);
        pump (50);
        snapshot (*ed, snapDir, "editor_min_1100x640");
        ed->setSize (1180, 760);
        button (*ed, "LIBRARY")->triggerClick();
        fam->setSelected (0, true);
        pump (100);
        snapshot (*ed, snapDir, "editor_sub_1180x760");
        snapshot (*ed, snapDir, "editor_sub_1180x760_scale150", 1.5f);
        snapshot (*ed, snapDir, "editor_sub_1180x760_scale200", 2.0f);

        // destroy/recreate editor repeatedly while audio runs
        int reopened = 0;
        for (int i = 0; i < 5; ++i)
        {
            proc.editorBeingDeleted (ed.get()); // what every plugin wrapper does before deleting the editor
            ed.reset();
            proc.processBlock (buf, midi);
            auto* fresh = proc.createEditorAndMakeActive();
            reopened += fresh != nullptr ? 1 : 0;
            ed.reset (fresh);
            pump (20);
            proc.processBlock (buf, midi);
        }
        std::printf ("    reopened %d/5\n", reopened);
        check (reopened == 5, "editor survives repeated close/reopen while audio runs");
        proc.editorBeingDeleted (ed.get());
        ed.reset();
    }
    std::printf ("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
