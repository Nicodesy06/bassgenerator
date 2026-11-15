#include "ui/PianoRoll.h"

#include "core/ScaleTheory.h"
#include "ui/DesignSystem.h"

namespace gfb::ui
{
namespace
{
constexpr int kKeysW = 38;
constexpr int kVelH = 34;
constexpr int kRulerH = 18;
} // namespace

PianoRoll::PianoRoll()
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    setTooltip ("Generated pattern (exact exported MIDI). Drag it into your DAW to drop a .mid clip. Ctrl+wheel zooms, wheel scrolls when zoomed, double-click resets zoom.");
}

void PianoRoll::setPattern (const Pattern& p)
{
    pattern = p;
    int lo = 127, hi = 0;
    for (auto& n : p.notes) { lo = std::min<int> (lo, n.midiNote); hi = std::max<int> (hi, n.midiNote); }
    if (p.notes.empty()) { lo = 36; hi = 48; }
    // show at least 1.5 octaves, padded, aligned to the root
    const int root = p.settings.root;
    lowNote = std::max (0, lo - 3);
    highNote = std::min (127, std::max (hi + 3, lowNote + 18));
    while (((lowNote - root) % 12 + 12) % 12 != 0 && lowNote > 0 && lo - lowNote < 8) --lowNote;
    hoveredIndex = -1;
    repaint();
}

void PianoRoll::setPlayheadTick (double t)
{
    if (std::abs (t - playhead) < 0.5) return;
    playhead = t;
    repaint();
}

juce::Rectangle<int> PianoRoll::rollArea() const { return getLocalBounds().withTrimmedLeft (kKeysW).withTrimmedTop (kRulerH).withTrimmedBottom (kVelH + 4); }
juce::Rectangle<int> PianoRoll::velocityArea() const { return getLocalBounds().withTrimmedLeft (kKeysW).removeFromBottom (kVelH); }

float PianoRoll::tickToX (double tick) const
{
    const auto r = rollArea();
    const double len = (double) std::max<int64_t> (pattern.lengthTicks(), kTicksPerBar);
    const double visible = len / zoom;
    const double start = scroll * (len - visible);
    return (float) r.getX() + (float) ((tick - start) / visible * r.getWidth());
}

double PianoRoll::xToTick (float x) const
{
    const auto r = rollArea();
    const double len = (double) std::max<int64_t> (pattern.lengthTicks(), kTicksPerBar);
    const double visible = len / zoom;
    return scroll * (len - visible) + (x - (float) r.getX()) / (float) r.getWidth() * visible;
}

float PianoRoll::pitchToY (int midi) const
{
    const auto r = rollArea();
    const float rowH = (float) r.getHeight() / (float) (highNote - lowNote + 1);
    return (float) r.getBottom() - (float) (midi - lowNote + 1) * rowH;
}

juce::Rectangle<float> PianoRoll::noteBounds (const NoteEvent& n) const
{
    const auto r = rollArea();
    const float rowH = (float) r.getHeight() / (float) (highNote - lowNote + 1);
    const float x0 = tickToX ((double) n.startTick), x1 = tickToX ((double) n.endTick());
    return { x0, pitchToY (n.midiNote), std::max (2.0f, x1 - x0), rowH };
}

void PianoRoll::paint (juce::Graphics& g)
{
    using namespace colours;
    g.fillAll (panel);
    const auto r = rollArea();
    const float rowH = (float) r.getHeight() / (float) (highNote - lowNote + 1);
    const int root = pattern.settings.root;
    const auto scale = pattern.settings.scale;

    // rows: scale tones lighter, root rows highlighted
    for (int m = lowNote; m <= highNote; ++m)
    {
        const float y = pitchToY (m);
        const int pc = m % 12;
        const bool inKey = theory::inScale (root, scale, pc);
        const bool isRoot = ((pc - root) % 12 + 12) % 12 == 0;
        // out-of-key rows get a quiet tint, the root row a hairline
        if (! inKey)
        {
            g.setColour (panel2);
            g.fillRect ((float) r.getX(), y, (float) r.getWidth(), rowH);
        }
        if (isRoot)
        {
            g.setColour (rule);
            g.fillRect ((float) r.getX(), y + rowH - 1.0f, (float) r.getWidth(), 1.0f);
        }
        // keyboard column: black keys as short ink bars
        const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        if (black)
        {
            g.setColour (ink.withAlpha (0.85f));
            g.fillRect ((float) kKeysW - 12.0f, y + 1.0f, 8.0f, rowH - 2.0f);
        }
        if (isRoot || pc == 0)
        {
            g.setColour (isRoot ? ink : textDim);
            g.setFont (font (juce::jlimit (8.0f, 11.0f, rowH * 0.85f), isRoot));
            g.drawText (juce::String (theory::noteName (m, root, scale)), juce::Rectangle<float> (2.0f, y, (float) kKeysW - 4.0f, rowH),
                        juce::Justification::centredLeft);
        }
    }
    // vertical grid: 16ths, beats, bars
    const int64_t len = std::max<int64_t> (pattern.lengthTicks(), kTicksPerBar);
    for (int64_t t = 0; t <= len; t += kTicksPerSixteenth)
    {
        const float x = tickToX ((double) t);
        if (x < (float) r.getX() - 1 || x > (float) r.getRight() + 1) continue;
        const bool bar = t % kTicksPerBar == 0, beat = t % kPPQ == 0;
        g.setColour (bar ? ink : (beat ? rule : grid));
        g.drawVerticalLine ((int) x, (float) r.getY() - (bar ? (float) kRulerH : 0.0f), (float) getHeight());
        if (bar && t < len)
        {
            g.setColour (ink);
            g.setFont (font (11.0f, true));
            g.drawText (juce::String (t / kTicksPerBar + 1), juce::Rectangle<float> (x + 4.0f, 0.0f, 40.0f, (float) kRulerH), juce::Justification::centredLeft);
        }
    }
    // notes
    g.saveState();
    g.reduceClipRegion (r.withBottom (getHeight()));
    for (size_t i = 0; i < pattern.notes.size(); ++i)
    {
        const auto& n = pattern.notes[i];
        auto b = noteBounds (n).reduced (0.5f, 1.0f);
        const float velA = 0.55f + 0.45f * (float) n.velocity / 127.0f;
        auto c = noteColour.withAlpha (velA);
        if ((int) i == hoveredIndex) c = focus;
        g.setColour (c);
        g.fillRect (b);
        if (n.accent)
        {
            // accent: a red bar on top of the note
            g.setColour (noteColour == red ? ink : red);
            g.fillRect (b.withHeight (juce::jmax (2.0f, b.getHeight() * 0.28f)));
        }
        if (n.slideToNext && i + 1 < pattern.notes.size())
        {
            const auto nb = noteBounds (pattern.notes[i + 1]);
            juce::Path p;
            p.startNewSubPath (b.getRight() - 2.0f, b.getCentreY());
            p.quadraticTo (nb.getX(), b.getCentreY(), nb.getX() + 3.0f, nb.getCentreY());
            g.setColour (noteColour);
            g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        // velocity lane
        const auto va = velocityArea();
        const float vh = (float) va.getHeight() * (float) n.velocity / 127.0f;
        const float vx = tickToX ((double) n.startTick);
        g.setColour (n.accent ? red : ink);
        g.fillRect (vx, (float) va.getBottom() - vh, 2.0f, vh);
    }
    g.restoreState();
    // velocity lane frame
    g.setColour (rule);
    g.drawHorizontalLine (velocityArea().getY() - 2, (float) kKeysW, (float) getWidth());
    g.setColour (textDim);
    g.setFont (font (10.0f));
    g.drawText ("Vel", juce::Rectangle<int> (0, velocityArea().getY(), kKeysW - 4, kVelH), juce::Justification::centred);

    // playhead
    if (playhead >= 0.0)
    {
        const float x = tickToX (playhead);
        if (x >= (float) r.getX() && x <= (float) r.getRight())
        {
            g.setColour (red);
            g.fillRect (x - 1.0f, 0.0f, 2.0f, (float) getHeight());
        }
    }
    // hover info
    if (hoveredIndex >= 0 && hoveredIndex < (int) pattern.notes.size())
    {
        const auto& n = pattern.notes[(size_t) hoveredIndex];
        const int bar = (int) (n.startTick / kTicksPerBar) + 1;
        const int beat = (int) ((n.startTick % kTicksPerBar) / kPPQ) + 1;
        const int six = (int) ((n.startTick % kPPQ) / kTicksPerSixteenth) + 1;
        const auto txt = juce::String (theory::noteName (n.midiNote, root, scale)) + "  " + juce::String (bar) + "." + juce::String (beat) + "." + juce::String (six)
                       + "  tick " + juce::String (n.startTick) + "  len " + juce::String (n.durationTicks) + "  vel " + juce::String (n.velocity)
                       + (n.accent ? "  ACC" : "") + (n.slideToNext ? "  SLIDE" : "");
        auto box = juce::Rectangle<int> (r.getRight() - 330, r.getY() + 4, 326, 20);
        g.setColour (paper);
        g.fillRect (box);
        g.setColour (ink);
        g.drawRect (box, 1);
        g.setFont (font (12.0f));
        g.drawText (txt, box.reduced (6, 0), juce::Justification::centredLeft);
    }
    // drag hint
    g.setColour (textDim.withAlpha (0.8f));
    g.setFont (font (11.0f));
    g.drawText (juce::String ("Drag to DAW"), getLocalBounds().removeFromTop (kRulerH).withTrimmedRight (6), juce::Justification::centredRight);
    g.setColour (ink);
    g.fillRect (0, 0, getWidth(), 1);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

void PianoRoll::mouseMove (const juce::MouseEvent& e)
{
    int hit = -1;
    for (size_t i = 0; i < pattern.notes.size(); ++i)
        if (noteBounds (pattern.notes[i]).expanded (1.0f, 0.0f).contains (e.position)) { hit = (int) i; break; }
    if (hit != hoveredIndex) { hoveredIndex = hit; repaint(); }
}

void PianoRoll::mouseExit (const juce::MouseEvent&)
{
    if (hoveredIndex != -1) { hoveredIndex = -1; repaint(); }
}

void PianoRoll::mouseDown (const juce::MouseEvent&) { dragStarted = false; }

void PianoRoll::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 8) return;
    dragStarted = true;
    if (onExternalDrag) onExternalDrag();
}

void PianoRoll::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.mods.isCommandDown())
    {
        zoom = juce::jlimit (1.0, 8.0, zoom * (w.deltaY > 0 ? 1.25 : 0.8));
        if (zoom <= 1.0) scroll = 0.0;
    }
    else if (zoom > 1.0)
        scroll = juce::jlimit (0.0, 1.0, scroll - (double) (std::abs (w.deltaY) > 0.0f ? w.deltaY : w.deltaX) * 0.3 / zoom);
    else
    {
        Component::mouseWheelMove (e, w);
        return;
    }
    repaint();
}

void PianoRoll::mouseDoubleClick (const juce::MouseEvent&)
{
    zoom = 1.0;
    scroll = 0.0;
    repaint();
}
} // namespace gfb::ui
