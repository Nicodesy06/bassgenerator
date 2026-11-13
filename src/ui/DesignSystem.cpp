#include "ui/DesignSystem.h"

namespace gfb::ui
{
juce::Font font (float h, bool bold)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Colour familyColour (bool acid) { return acid ? colours::acid : colours::sub; }

ForgeLookAndFeel::ForgeLookAndFeel()
{
    using namespace colours;
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::ComboBox::backgroundColourId, panel2);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, outline);
    setColour (juce::ComboBox::arrowColourId, textDim);
    setColour (juce::ComboBox::focusedOutlineColourId, focus);
    setColour (juce::PopupMenu::backgroundColourId, panel2);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, forge.withAlpha (0.85f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::TextButton::buttonColourId, raised);
    setColour (juce::TextButton::buttonOnColourId, forge);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::Label::textColourId, text);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, forge);
    setColour (juce::Slider::trackColourId, forge);
    setColour (juce::Slider::backgroundColourId, panel2);
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::TextEditor::backgroundColourId, bg);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, outline);
    setColour (juce::TextEditor::focusedOutlineColourId, focus);
    setColour (juce::TextEditor::highlightColourId, forge.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, forge);
    setColour (juce::ListBox::backgroundColourId, panel);
    setColour (juce::ListBox::outlineColourId, outline);
    setColour (juce::TooltipWindow::backgroundColourId, raised);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::TabbedComponent::backgroundColourId, panel);
    setColour (juce::TabbedComponent::outlineColourId, outline);
    setColour (juce::TabbedButtonBar::tabOutlineColourId, outline);
    setColour (juce::ScrollBar::thumbColourId, outline);
    setColour (juce::AlertWindow::backgroundColourId, panel2);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::AlertWindow::outlineColourId, outline);
    setColour (juce::ToggleButton::textColourId, text);
    setColour (juce::ToggleButton::tickColourId, forge);
    setColour (juce::ToggleButton::tickDisabledColourId, textDim);
}

void ForgeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float track = juce::jmax (2.5f, r * 0.13f);

    juce::Path bgArc;
    bgArc.addCentredArc (c.x, c.y, r - track, r - track, 0, a0, a1, true);
    g.setColour (colours::outline);
    g.strokePath (bgArc, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path valArc;
    valArc.addCentredArc (c.x, c.y, r - track, r - track, 0, a0, ang, true);
    g.setColour (s.isEnabled() ? accent : colours::textDim);
    g.strokePath (valArc, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float kr = (r - track * 2.2f);
    g.setGradientFill (juce::ColourGradient (colours::raised.brighter (0.15f), c.x, c.y - kr, colours::panel2, c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (s.hasKeyboardFocus (true) ? colours::focus : colours::outline);
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    juce::Path tick;
    tick.addRoundedRectangle (-1.6f, -kr + 3.0f, 3.2f, kr * 0.48f, 1.6f);
    g.setColour (colours::text);
    g.fillPath (tick, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void ForgeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto c = on ? b.findColour (juce::TextButton::buttonOnColourId) : base;
    if (! b.isEnabled()) c = c.withMultipliedAlpha (0.4f);
    else if (down) c = c.darker (0.25f);
    else if (over) c = c.brighter (0.12f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (b.hasKeyboardFocus (false) ? colours::focus : colours::outline.withAlpha (on ? 0.0f : 1.0f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void ForgeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 2), juce::Justification::centred, 1, 0.55f);
}

juce::Font ForgeLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return font (juce::jlimit (11.0f, 17.0f, (float) h * 0.42f), true); }
juce::Font ForgeLookAndFeel::getComboBoxFont (juce::ComboBox& c) { return font (juce::jlimit (12.0f, 16.0f, (float) c.getHeight() * 0.48f), true); }
juce::Font ForgeLookAndFeel::getPopupMenuFont() { return font (14.0f); }

void ForgeLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.hasKeyboardFocus (true) ? colours::focus : colours::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (colours::textDim);
    g.fillPath (arrow);
}

void ForgeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void ForgeLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& t, int w, int h)
{
    g.setColour (colours::raised);
    g.fillRoundedRectangle (0, 0, (float) w, (float) h, 4.0f);
    g.setColour (colours::outline);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1, (float) h - 1, 4.0f, 1.0f);
    g.setColour (colours::text);
    g.setFont (font (13.0f));
    g.drawFittedText (t, juce::Rectangle<int> (w, h).reduced (6, 3), juce::Justification::centredLeft, 6);
}

void ForgeLookAndFeel::drawTabButton (juce::TabBarButton& b, juce::Graphics& g, bool over, bool)
{
    const bool front = b.isFrontTab();
    auto r = b.getLocalBounds().toFloat().reduced (2.0f, 3.0f);
    g.setColour (front ? colours::raised : (over ? colours::panel2 : colours::panel));
    g.fillRoundedRectangle (r, 4.0f);
    if (front)
    {
        g.setColour (colours::forge);
        g.fillRect (r.removeFromBottom (2.0f));
    }
    g.setColour (front ? colours::text : colours::textDim);
    g.setFont (font (13.0f, true));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void ForgeLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int ts, int tsz, bool over, bool)
{
    auto r = vertical ? juce::Rectangle<int> (x + 2, ts, w - 4, tsz) : juce::Rectangle<int> (ts, y + 2, tsz, h - 4);
    g.setColour (over ? colours::textDim : colours::outline);
    g.fillRoundedRectangle (r.toFloat(), 3.0f);
}

LockButton::LockButton() : juce::Button ("lock")
{
    setClickingTogglesState (true);
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void LockButton::paintButton (juce::Graphics& g, bool over, bool)
{
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const float s = juce::jmin (r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre (s, s);
    const auto col = on ? colours::forge : (over ? colours::text : colours::textDim.withAlpha (0.7f));
    auto body = r.withTrimmedTop (s * 0.45f).reduced (s * 0.12f, 0.0f);
    g.setColour (col);
    g.fillRoundedRectangle (body, 2.0f);
    juce::Path shackle;
    const float sx = body.getX() + body.getWidth() * 0.2f, sw = body.getWidth() * 0.6f;
    const float top = r.getY() + s * 0.08f;
    shackle.startNewSubPath (sx, body.getY());
    shackle.lineTo (sx, top + sw * 0.5f);
    shackle.addArc (sx, top, sw, sw, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, false);
    shackle.lineTo (sx + sw, on ? body.getY() : body.getY() - s * 0.12f);
    g.strokePath (shackle, juce::PathStrokeType (juce::jmax (1.4f, s * 0.11f)));
    if (hasKeyboardFocus (false))
    {
        g.setColour (colours::focus);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.0f, 1.0f);
    }
}

Segmented::Segmented (juce::StringArray it) : items (std::move (it))
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Segmented::setSelected (int i, bool notify)
{
    i = juce::jlimit (0, items.size() - 1, i);
    if (i == selected) return;
    selected = i;
    repaint();
    if (notify && onChange) onChange (selected);
}

void Segmented::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colours::panel2);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (hasKeyboardFocus (false) ? colours::focus : colours::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    const float w = r.getWidth() / (float) items.size();
    for (int i = 0; i < items.size(); ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
        if (i == selected)
        {
            g.setColour (accent);
            g.fillRoundedRectangle (seg, 4.0f);
        }
        g.setColour (i == selected ? juce::Colours::black : colours::textDim);
        g.setFont (font (juce::jlimit (11.0f, 15.0f, r.getHeight() * 0.45f), true));
        g.drawFittedText (items[i], seg.toNearestInt().reduced (1, 0), juce::Justification::centred, 1, 0.6f);
    }
}

void Segmented::mouseDown (const juce::MouseEvent& e)
{
    const int i = (int) ((float) e.x / ((float) getWidth() / (float) items.size()));
    setSelected (i, true);
}

bool Segmented::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::leftKey) { setSelected (selected - 1, true); return true; }
    if (k == juce::KeyPress::rightKey) { setSelected (selected + 1, true); return true; }
    return false;
}

void drawCaption (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& t)
{
    g.setColour (colours::textDim);
    g.setFont (font (11.0f, true));
    g.drawText (t.toUpperCase(), area, juce::Justification::centredLeft);
}
} // namespace gfb::ui
