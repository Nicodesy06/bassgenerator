#pragma once
// GrooveForge visual identity: dark graphite panels, warm "forge" orange
// for primary actions, amber for SUB, acid green for ACID. All drawing is
// vector based (scales cleanly with Windows DPI / host scale factors).

#include <juce_gui_basics/juce_gui_basics.h>

namespace gfb::ui
{
namespace colours
{
inline const juce::Colour bg { 0xff0e1014 };
inline const juce::Colour panel { 0xff161920 };
inline const juce::Colour panel2 { 0xff1d212a };
inline const juce::Colour raised { 0xff252a35 };
inline const juce::Colour outline { 0xff2e3441 };
inline const juce::Colour grid { 0xff232833 };
inline const juce::Colour text { 0xffe9ebf1 };
inline const juce::Colour textDim { 0xff8a92a5 };
inline const juce::Colour forge { 0xffff7a2f };
inline const juce::Colour sub { 0xffffb648 };
inline const juce::Colour acid { 0xffb4ee3c };
inline const juce::Colour danger { 0xffff5470 };
inline const juce::Colour focus { 0xff5ab4ff };
} // namespace colours

juce::Font font (float height, bool bold = false);
juce::Colour familyColour (bool acid);

class ForgeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ForgeLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int w, int h) override;
    void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool over, bool down) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override;
};

/** Small padlock toggle used next to every lockable setting. */
class LockButton : public juce::Button
{
public:
    LockButton();
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

/** Row of mutually exclusive segments (Bars, Family). */
class Segmented : public juce::Component, public juce::SettableTooltipClient
{
public:
    Segmented (juce::StringArray items);
    void setSelected (int index, bool notify);
    int getSelected() const noexcept { return selected; }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    std::function<void (int)> onChange;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }

private:
    juce::StringArray items;
    int selected = 0;
    juce::Colour accent = colours::forge;
};

/** Section caption + child, used to label controls. */
void drawCaption (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text);
} // namespace gfb::ui
