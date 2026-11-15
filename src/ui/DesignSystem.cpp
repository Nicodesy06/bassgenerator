#include "ui/DesignSystem.h"

namespace gfb::ui
{
namespace
{
juce::String uiTypefaceName()
{
   #if JUCE_MAC
    return "Helvetica Neue";
   #elif JUCE_WINDOWS
    return "Arial";
   #else
    return "Liberation Sans";
   #endif
}

juce::String monoTypefaceName()
{
   #if JUCE_MAC
    return "Menlo";
   #elif JUCE_WINDOWS
    return "Consolas";
   #else
    return "DejaVu Sans Mono";
   #endif
}
} // namespace

juce::Font font (float h, bool b)
{
    return juce::Font (juce::FontOptions (uiTypefaceName(), h, b ? juce::Font::bold : juce::Font::plain));
}

juce::Font monoFont (float h, bool b)
{
    return juce::Font (juce::FontOptions (monoTypefaceName(), h, b ? juce::Font::bold : juce::Font::plain));
}

juce::Colour familyColour (bool acid) { return acid ? colours::acid : colours::sub; }

ForgeLookAndFeel::ForgeLookAndFeel()
{
    using namespace colours;
    setColour (juce::ResizableWindow::backgroundColourId, paper);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::outlineColourId, ink);
    setColour (juce::ComboBox::arrowColourId, ink);
    setColour (juce::ComboBox::focusedOutlineColourId, focus);
    setColour (juce::PopupMenu::backgroundColourId, paper);
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ink);
    setColour (juce::PopupMenu::highlightedTextColourId, paper);
    setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonOnColourId, ink);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, paper);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, ink);
    setColour (juce::Slider::trackColourId, ink);
    setColour (juce::Slider::backgroundColourId, rule);
    setColour (juce::Slider::thumbColourId, ink);
    setColour (juce::TextEditor::backgroundColourId, paper);
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::outlineColourId, ink);
    setColour (juce::TextEditor::focusedOutlineColourId, focus);
    setColour (juce::TextEditor::highlightColourId, red.withAlpha (0.25f));
    setColour (juce::TextEditor::highlightedTextColourId, ink);
    setColour (juce::CaretComponent::caretColourId, red);
    setColour (juce::ListBox::backgroundColourId, paper);
    setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, paper);
    setColour (juce::TooltipWindow::textColourId, ink);
    setColour (juce::TabbedComponent::backgroundColourId, paper);
    setColour (juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId, rule);
    setColour (juce::AlertWindow::backgroundColourId, paper);
    setColour (juce::AlertWindow::textColourId, ink);
    setColour (juce::AlertWindow::outlineColourId, ink);
    setColour (juce::ToggleButton::textColourId, ink);
    setColour (juce::ToggleButton::tickColourId, ink);
    setColour (juce::ToggleButton::tickDisabledColourId, textDim);
}

juce::Typeface::Ptr ForgeLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    // Map JUCE's default sans-serif to the grotesque used everywhere else.
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
    {
        juce::Font g (juce::FontOptions (uiTypefaceName(), f.getHeight(), f.getStyleFlags()));
        return juce::LookAndFeel_V4::getTypefaceForFont (g);
    }
    return juce::LookAndFeel_V4::getTypefaceForFont (f);
}

void ForgeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearHorizontal && style != juce::Slider::LinearBar)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s);
        return;
    }
    using namespace colours;
    const auto r = juce::Rectangle<int> (x, y, w, h).toFloat();
    const auto accent = s.findColour (juce::Slider::trackColourId);
    if (s.getTextBoxPosition() == juce::Slider::NoTextBox)
    {
        // value, top right (caption is a label at top left)
        g.setColour (s.isEnabled() ? ink : textDim);
        g.setFont (font (juce::jlimit (11.0f, 14.0f, r.getHeight() * 0.36f), true));
        g.drawText (s.getTextFromValue (s.getValue()), r.withHeight (juce::jmin (16.0f, r.getHeight() * 0.5f)), juce::Justification::centredRight);
    }
    // hairline track + solid value bar
    const float ty = s.getTextBoxPosition() == juce::Slider::NoTextBox ? r.getBottom() - juce::jmax (6.0f, r.getHeight() * 0.25f) : r.getCentreY();
    g.setColour (rule);
    g.fillRect (r.getX(), ty - 0.5f, r.getWidth(), 1.0f);
    g.setColour (s.isEnabled() ? accent : textDim);
    g.fillRect (r.getX(), ty - 1.5f, juce::jmax (1.0f, pos - r.getX()), 3.0f);
    g.fillRect (pos - 1.5f, ty - 6.0f, 3.0f, 12.0f);
    if (s.hasKeyboardFocus (false))
    {
        g.setColour (focus);
        g.drawRect (r, 1.0f);
    }
}

void ForgeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    // kept for completeness (minimal ring + tick); the plug-in UI uses linear sliders
    const auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    g.setColour (colours::rule);
    g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.0f);
    const float ang = a0 + pos * (a1 - a0);
    g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
    juce::Path p;
    p.addCentredArc (c.x, c.y, r, r, 0, a0, ang, true);
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

void ForgeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    using namespace colours;
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const bool filled = on || base.getAlpha() > 0; // explicit fill = primary button
    juce::Colour fill = on ? b.findColour (juce::TextButton::buttonOnColourId) : base;
    if (! filled) fill = down ? raised : (over ? panel2 : juce::Colours::transparentBlack);
    else if (over || down) fill = (fill == red ? ink : red); // Swiss hover: swap to the other ink
    if (! b.isEnabled()) fill = fill.withMultipliedAlpha (0.4f);
    g.setColour (fill);
    g.fillRect (r);
    g.setColour (b.hasKeyboardFocus (false) ? focus : (b.isEnabled() ? ink : rule));
    g.drawRect (r, 1.0f);
}

void ForgeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool filled = b.getToggleState() || b.findColour (juce::TextButton::buttonColourId).getAlpha() > 0;
    g.setFont (getTextButtonFont (b, b.getHeight()));
    auto c = filled ? b.findColour (juce::TextButton::textColourOnId) : b.findColour (juce::TextButton::textColourOffId);
    g.setColour (c.withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.4f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 2), juce::Justification::centred, 1, 0.55f);
}

juce::Font ForgeLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return font (juce::jlimit (11.0f, 15.0f, (float) h * 0.4f), true); }
juce::Font ForgeLookAndFeel::getComboBoxFont (juce::ComboBox& c) { return font (juce::jlimit (13.0f, 17.0f, (float) c.getHeight() * 0.5f), true); }
juce::Font ForgeLookAndFeel::getPopupMenuFont() { return font (14.0f); }
juce::Font ForgeLookAndFeel::getLabelFont (juce::Label& l) { return font (juce::jlimit (11.0f, 16.0f, (float) l.getHeight() * 0.62f)); }

void ForgeLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    using namespace colours;
    // no box: a single ink underline (focus = blue)
    g.setColour (box.hasKeyboardFocus (true) ? focus : ink);
    g.fillRect (0.0f, (float) h - 2.0f, (float) w, box.hasKeyboardFocus (true) ? 2.0f : 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 9.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.isEnabled() ? ink : textDim);
    g.fillPath (arrow);
}

void ForgeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (0, 1, box.getWidth() - 18, box.getHeight() - 3);
    label.setFont (getComboBoxFont (box));
}

void ForgeLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (colours::paper);
    g.setColour (colours::ink);
    g.drawRect (0, 0, w, h, 1);
}

void ForgeLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& t, int w, int h)
{
    g.fillAll (colours::paper);
    g.setColour (colours::ink);
    g.drawRect (0, 0, w, h, 1);
    g.setFont (font (13.0f));
    g.drawFittedText (t, juce::Rectangle<int> (w, h).reduced (8, 4), juce::Justification::centredLeft, 6);
}

void ForgeLookAndFeel::drawTabButton (juce::TabBarButton& b, juce::Graphics& g, bool over, bool)
{
    using namespace colours;
    const bool front = b.isFrontTab();
    auto r = b.getLocalBounds();
    g.setColour (front ? ink : (over ? ink : textDim));
    g.setFont (font (13.0f, front));
    g.drawText (b.getButtonText(), r.reduced (2, 0), juce::Justification::centredLeft);
    if (front) g.fillRect (r.removeFromBottom (2).withWidth (juce::jmin (r.getWidth(), (int) juce::GlyphArrangement::getStringWidth (font (13.0f, true), b.getButtonText()) + 4)).translated (2, 0));
}

void ForgeLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int ts, int tsz, bool over, bool)
{
    auto r = vertical ? juce::Rectangle<int> (x + w - 3, ts, 3, tsz) : juce::Rectangle<int> (ts, y + h - 3, tsz, 3);
    g.setColour (over ? colours::ink : colours::rule);
    g.fillRect (r);
}

void ForgeLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (colours::paper);
    g.fillRect (0, 0, w, h);
}

void ForgeLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    g.setColour (e.hasKeyboardFocus (true) ? colours::focus : colours::ink);
    g.fillRect (0, h - 1, w, 1);
}

LockButton::LockButton() : juce::Button ("lock")
{
    setClickingTogglesState (true);
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void LockButton::paintButton (juce::Graphics& g, bool over, bool)
{
    using namespace colours;
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    const float s = juce::jmin (r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre (s, s);
    const auto col = on ? red : (over ? ink : textDim);
    auto body = r.withTrimmedTop (s * 0.45f).reduced (s * 0.14f, 0.0f);
    g.setColour (col);
    if (on) g.fillRect (body); else g.drawRect (body, 1.2f);
    juce::Path shackle;
    const float sx = body.getX() + body.getWidth() * 0.22f, sw = body.getWidth() * 0.56f;
    const float top = r.getY() + s * 0.06f;
    shackle.startNewSubPath (sx, body.getY());
    shackle.lineTo (sx, top + sw * 0.5f);
    shackle.addArc (sx, top, sw, sw, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, false);
    shackle.lineTo (sx + sw, on ? body.getY() : body.getY() - s * 0.14f);
    g.strokePath (shackle, juce::PathStrokeType (1.3f));
    if (hasKeyboardFocus (false))
    {
        g.setColour (focus);
        g.drawRect (getLocalBounds(), 1);
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
    using namespace colours;
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const float w = r.getWidth() / (float) items.size();
    for (int i = 0; i < items.size(); ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
        if (i == selected)
        {
            g.setColour (accent);
            g.fillRect (seg);
        }
        g.setColour (i == selected ? paper : ink);
        g.setFont (font (juce::jlimit (11.0f, 15.0f, r.getHeight() * 0.44f), true));
        g.drawFittedText (items[i], seg.toNearestInt().reduced (1, 0), juce::Justification::centred, 1, 0.6f);
    }
    g.setColour (hasKeyboardFocus (false) ? focus : ink);
    g.drawRect (r, 1.0f);
    for (int i = 1; i < items.size(); ++i) g.fillRect (r.getX() + w * (float) i - 0.5f, r.getY(), 1.0f, r.getHeight());
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
    g.setFont (font (11.5f));
    g.drawText (t, area, juce::Justification::centredLeft);
}

void drawSection (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& number, const juce::String& title)
{
    g.setColour (colours::ink);
    g.fillRect (area.getX(), area.getY(), area.getWidth(), 2);
    auto t = area.withTrimmedTop (4).withHeight (18);
    g.setFont (font (12.0f, true));
    g.drawText (number, t.removeFromLeft (26), juce::Justification::centredLeft);
    g.drawText (title, t, juce::Justification::centredLeft);
}
} // namespace gfb::ui
