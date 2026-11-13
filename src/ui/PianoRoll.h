#pragma once
// Piano roll that draws the exact canonical NoteEvents (tick positions,
// durations incl. swing offsets, velocity, accent, slide). Also the drag
// source for native external MIDI drag & drop.

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/PatternTypes.h"

namespace gfb::ui
{
class PianoRoll : public juce::Component, public juce::SettableTooltipClient
{
public:
    PianoRoll();
    void setPattern (const Pattern& p);
    void setPlayheadTick (double tick); // < 0 = hidden
    void setAccentColour (juce::Colour c) { noteColour = c; repaint(); }
    /** Called when the user drags out of the roll: must create a file and start the OS drag. */
    std::function<void()> onExternalDrag;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    juce::Rectangle<float> noteBounds (const NoteEvent& n) const; // for tests/hit testing

private:
    juce::Rectangle<int> rollArea() const;
    juce::Rectangle<int> velocityArea() const;
    float tickToX (double tick) const;
    double xToTick (float x) const;
    float pitchToY (int midi) const;
    int hoveredIndex = -1;
    Pattern pattern;
    int lowNote = 24, highNote = 48;
    double playhead = -1.0;
    double zoom = 1.0, scroll = 0.0; // horizontal zoom (1..8) and scroll (0..1)
    bool dragStarted = false;
    juce::Colour noteColour { 0xffffb648 };
};
} // namespace gfb::ui
