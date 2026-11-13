#include "ui/SeedBox.h"

#include "core/GeneratorSettings.h"
#include "ui/DesignSystem.h"

namespace gfb::ui
{
SeedBox::SeedBox (juce::String cap) : caption (std::move (cap))
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTooltip (caption + " seed. Drag up/down (Shift = fine), mouse wheel, arrow keys. Double-click to type. Right-click: copy / paste / random.");
}

void SeedBox::setValue (uint64_t v, bool notify)
{
    if (v == value) return;
    value = v;
    repaint();
    if (notify && onChange) onChange (value);
}

void SeedBox::step (int64_t d)
{
    // wraps like a counter; uint64 arithmetic is well defined
    setValue (value + (uint64_t) d, true);
}

void SeedBox::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colours::bg);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (hasKeyboardFocus (true) ? colours::focus : (dragging ? accent : colours::outline));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    auto area = getLocalBounds().reduced (8, 2);
    g.setColour (accent);
    g.setFont (font (11.0f, true));
    g.drawText (caption, area.removeFromLeft (44), juce::Justification::centredLeft);
    if (editor == nullptr)
    {
        g.setColour (colours::text);
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), juce::jlimit (13.0f, 18.0f, (float) getHeight() * 0.52f), juce::Font::bold)));
        g.drawFittedText (juce::String (seedToString (value)), area, juce::Justification::centredRight, 1, 0.6f);
    }
}

void SeedBox::resized()
{
    if (editor) editor->setBounds (getLocalBounds().reduced (50, 3).withTrimmedRight (-44));
}

void SeedBox::mouseDown (const juce::MouseEvent& e)
{
    if (editor) return;
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addItem (1, "Copy seed");
        m.addItem (2, "Paste seed");
        m.addItem (3, "Random seed");
        m.addItem (4, "Type value...");
        juce::Component::SafePointer<SeedBox> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r) {
            if (safe == nullptr) return;
            if (r == 1) safe->copyToClipboard();
            if (r == 2) safe->pasteFromClipboard();
            if (r == 3) safe->setValue ((uint64_t) juce::Random::getSystemRandom().nextInt (juce::Range<int> (1, 1000000)), true);
            if (r == 4) safe->beginEdit();
        });
        return;
    }
    dragging = true;
    dragAccum = 0;
    lastDragY = e.y;
    dragStartValue = value;
    repaint();
}

void SeedBox::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const float pxPerStep = e.mods.isShiftDown() ? 14.0f : 3.0f;
    dragAccum += (float) (lastDragY - e.y);
    lastDragY = e.y;
    const int steps = (int) (dragAccum / pxPerStep);
    if (steps != 0)
    {
        dragAccum -= (float) steps * pxPerStep;
        step (steps);
    }
}

void SeedBox::mouseUp (const juce::MouseEvent&) { dragging = false; repaint(); }

void SeedBox::mouseDoubleClick (const juce::MouseEvent&) { beginEdit(); }

void SeedBox::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    wheelAccum += (double) (std::abs (w.deltaY) > 0.0f ? w.deltaY : w.deltaX) * (w.isReversed ? -1.0 : 1.0);
    const int notches = (int) (wheelAccum / 0.1);
    if (notches != 0)
    {
        wheelAccum -= notches * 0.1;
        step ((int64_t) notches * (e.mods.isCommandDown() ? 100 : 1));
    }
}

bool SeedBox::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::upKey) { step (1); return true; }
    if (k == juce::KeyPress::downKey) { step (-1); return true; }
    if (k == juce::KeyPress::pageUpKey) { step (100); return true; }
    if (k == juce::KeyPress::pageDownKey) { step (-100); return true; }
    if (k == juce::KeyPress::returnKey) { beginEdit(); return true; }
    if (k == juce::KeyPress ('c', juce::ModifierKeys::commandModifier, 0)) { copyToClipboard(); return true; }
    if (k == juce::KeyPress ('v', juce::ModifierKeys::commandModifier, 0)) { pasteFromClipboard(); return true; }
    return false;
}

void SeedBox::beginEdit()
{
    editor = std::make_unique<juce::TextEditor>();
    editor->setFont (font (15.0f, true));
    editor->setJustification (juce::Justification::centredRight);
    editor->setText (juce::String (seedToString (value)), false);
    editor->setInputRestrictions (20, "0123456789xXabcdefABCDEF");
    editor->onReturnKey = [this] { endEdit (true); };
    editor->onEscapeKey = [this] { endEdit (false); };
    editor->onFocusLost = [this] { endEdit (true); };
    addAndMakeVisible (*editor);
    resized();
    editor->grabKeyboardFocus();
    editor->selectAll();
    repaint();
}

void SeedBox::endEdit (bool commit)
{
    if (editor == nullptr) return;
    const auto text = editor->getText().toStdString();
    // endEdit can be invoked from the editor's own callbacks: detach now, delete asynchronously
    removeChildComponent (editor.get());
    std::shared_ptr<juce::TextEditor> doomed (editor.release());
    juce::MessageManager::callAsync ([doomed] {});
    if (commit)
        if (auto v = parseSeed (text)) setValue (*v, true); // invalid or overflowing input keeps the old value
    repaint();
}

void SeedBox::copyToClipboard() { juce::SystemClipboard::copyTextToClipboard (juce::String (seedToString (value))); }

void SeedBox::pasteFromClipboard()
{
    if (auto v = parseSeed (juce::SystemClipboard::getTextFromClipboard().toStdString())) setValue (*v, true);
}
} // namespace gfb::ui
