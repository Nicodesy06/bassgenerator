#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "state/StateCodec.h"
#include "ui/DesignSystem.h"
#include "ui/SeedBox.h"

namespace gfb
{
class GrooveForgeProcessor;
}

namespace gfb::ui
{
/** Top bar: Key, Scale, Family, Feel, Bars, Octave (+ locks), Generate, Randomise. */
class GeneratorPanel : public juce::Component
{
public:
    explicit GeneratorPanel (GrooveForgeProcessor& p);
    void refresh (const state::InstanceState& s);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void pushSettings();
    void pushLocks();
    GrooveForgeProcessor& proc;
    bool updating = false;
    GeneratorSettings current;
    juce::ComboBox root, scale, feel;
    Segmented family { { "SUB", "ACID" } }, bars { { "1", "2", "4", "8" } };
    juce::TextButton octDown { "-" }, octUp { "+" };
    juce::Label octLabel;
    LockButton lRoot, lScale, lFamily, lFeel, lBars, lOctave;
    juce::TextButton generateBtn { "GENERATE" }, diceBtn { "RANDOMISE" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> captions;
};

/** Seed row + workflow actions under the piano roll. */
class ActionBar : public juce::Component
{
public:
    explicit ActionBar (GrooveForgeProcessor& p);
    void refresh (const state::InstanceState& s, bool preview, bool sync, bool midiOut);
    void resized() override;
    void paint (juce::Graphics&) override;

    std::function<void()> onExport, onDragStart, onToggleBrowser, onAddFavorite;
    juce::TextButton browserBtn { "LIBRARY" };

private:
    GrooveForgeProcessor& proc;
    bool updating = false;
    SeedBox style { "STYLE" }, var { "VAR" };
    LockButton lStyle, lVar;
    juce::TextButton newStyle { "NEW STYLE" }, newVar { "NEW VAR" };
    juce::TextButton back { "<" }, fwd { ">" }, favBtn { "+ FAV" };
    juce::Label histLabel;
    juce::TextButton exportBtn { "EXPORT MIDI" }, dragBtn { "DRAG MIDI" };
    juce::TextButton previewBtn { "PREVIEW" }, syncBtn { "HOST SYNC" }, midiOutBtn { "MIDI OUT" };
    struct DragHandle : juce::MouseListener
    {
        std::function<void()> start;
        bool started = false;
        void mouseDown (const juce::MouseEvent&) override { started = false; }
        void mouseDrag (const juce::MouseEvent& e) override { if (! started && e.getDistanceFromDragStart() > 6) { started = true; if (start) start(); } }
    } dragHandle;
};
} // namespace gfb::ui
