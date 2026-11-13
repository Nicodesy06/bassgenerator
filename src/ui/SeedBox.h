#pragma once
// Editable 64-bit seed field.
//   drag up/down  : +-1 per 3 px (Shift: fine, 1 per 14 px)
//   mouse wheel   : +-1 per notch (Ctrl/Cmd: +-100)
//   double-click  : type a value (decimal or 0x hex; overflow rejected)
//   right-click   : copy / paste / randomise
//   keyboard      : Up/Down (+-1), PageUp/PageDown (+-100), Enter = edit, Ctrl+C / Ctrl+V

#include <juce_gui_basics/juce_gui_basics.h>

namespace gfb::ui
{
class SeedBox : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SeedBox (juce::String caption);
    void setValue (uint64_t v, bool notify);
    uint64_t getValue() const noexcept { return value; }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    std::function<void (uint64_t)> onChange;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }

private:
    void step (int64_t delta);
    void beginEdit();
    void endEdit (bool commit);
    void copyToClipboard();
    void pasteFromClipboard();

    juce::String caption;
    uint64_t value = 1, dragStartValue = 1;
    float dragAccum = 0;
    int lastDragY = 0;
    bool dragging = false;
    double wheelAccum = 0;
    juce::Colour accent { 0xffff7a2f };
    std::unique_ptr<juce::TextEditor> editor;
};
} // namespace gfb::ui
