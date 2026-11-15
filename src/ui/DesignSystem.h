#pragma once
// GrooveForge visual identity (since 0.3): Swiss / International style.
// Paper background, ink typography, one red accent, grotesque type
// (Helvetica / Arial), strict grid, hairline rules instead of boxes, square
// corners, no gradients or shadows. Vector drawing only (DPI independent).

#include <juce_gui_basics/juce_gui_basics.h>

namespace gfb::ui
{
namespace colours
{
inline const juce::Colour paper { 0xfff4f3ef };   // background
inline const juce::Colour bg = paper;
inline const juce::Colour panel = paper;
inline const juce::Colour panel2 { 0xffeae9e4 };  // quiet fill (hover, scale rows)
inline const juce::Colour raised { 0xffe0dfd9 };
inline const juce::Colour ink { 0xff111111 };     // text, rules, primary
inline const juce::Colour outline = ink;
inline const juce::Colour rule { 0xffcfcec8 };    // hairlines
inline const juce::Colour grid { 0xffe3e2dc };
inline const juce::Colour text = ink;
inline const juce::Colour textDim { 0xff7d7c77 };
inline const juce::Colour red { 0xffe3001b };     // the one accent
inline const juce::Colour forge = red;
inline const juce::Colour sub = ink;
inline const juce::Colour acid = red;
inline const juce::Colour danger = red;
inline const juce::Colour focus { 0xff0050ff };
} // namespace colours

/** Grotesque UI font (Helvetica Neue / Arial / Liberation Sans). */
juce::Font font (float height, bool bold = false);
juce::Font monoFont (float height, bool bold = false);
juce::Colour familyColour (bool acid);

class ForgeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ForgeLookAndFeel();
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont (juce::Label&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int w, int h) override;
    void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool over, bool down) override;
    void drawTabbedButtonBarBackground (juce::TabbedButtonBar&, juce::Graphics&) override {}
    void drawTabAreaBehindFrontButton (juce::TabbedButtonBar&, juce::Graphics&, int, int) override {}
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override;
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;

private:
    juce::Typeface::Ptr regular, bold;
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
    juce::Colour accent = colours::ink;
};

/** Caption above a control: small, grey, sentence case. */
void drawCaption (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text);
/** Section head: 2 px ink rule on top and a bold title (Swiss grid). */
void drawSection (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& number, const juce::String& title);
} // namespace gfb::ui
