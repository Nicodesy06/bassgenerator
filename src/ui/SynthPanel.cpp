#include "ui/SynthPanel.h"

#include "PluginProcessor.h"
#include "dsp/SynthParams.h"

namespace gfb::ui
{
namespace
{
struct KnobDef { const char* id; const char* label; const char* tip; };
const KnobDef kKnobs[] = {
    { "volume", "VOLUME", "Output level (smoothed)" },
    { "cutoff", "CUTOFF", "Filter cutoff (log, 30 Hz - 12 kHz)" },
    { "resonance", "RESO", "Filter resonance (bounded below self-oscillation)" },
    { "envmod", "ENV MOD", "Filter envelope depth" },
    { "decay", "DECAY", "Filter / amp envelope decay time" },
    { "accent", "ACCENT", "How strongly accented notes hit: level, filter sweep, drive" },
    { "drive", "DRIVE", "Saturation (2x oversampled, level compensated)" },
    { "glide", "GLIDE", "Slide (portamento) time for notes flagged slide" },
    { "shape", "SHAPE", "Oscillator shape: SUB harmonic layer amount / saw-square; ACID saw-square" },
};
} // namespace

SynthPanel::SynthPanel (GrooveForgeProcessor& p) : proc (p)
{
    for (const auto& d : kKnobs)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
        k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k->slider.setTooltip (juce::String (d.tip) + ". Double-click: default. Shift-drag: fine.");
        k->slider.setWantsKeyboardFocus (true);
        k->slider.setDoubleClickReturnValue (true, 0.0); // replaced by attachment default below
        k->slider.setVelocityModeParameters (1.0, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
        k->name.setText (d.label, juce::dontSendNotification);
        k->name.setJustificationType (juce::Justification::centred);
        k->name.setFont (font (11.0f, true));
        k->name.setColour (juce::Label::textColourId, colours::textDim);
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, k->slider);
        if (auto* prm = proc.apvts.getParameter (d.id))
            k->slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->name);
        knobs.push_back (std::move (k));
    }
    const auto presets = factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i) preset.addItem (juce::String (std::string (presets[(size_t) i].name)), i + 1);
    preset.setTooltip ("Factory sound presets (original patches)");
    preset.onChange = [this] { if (preset.getSelectedId() > 0) proc.applyPreset (preset.getSelectedId() - 1); };
    addAndMakeVisible (preset);
    engine.setTooltip ("Synth engine. Follows the generator family automatically; can be switched independently.");
    addAndMakeVisible (engine);
    if (auto* prm = proc.apvts.getParameter ("engine"))
    {
        engineAttachment = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v) {
            engine.setSelected (v > 0.5f ? 1 : 0, false);
            engine.setAccent (familyColour (v > 0.5f));
            for (auto& k : knobs) k->slider.setColour (juce::Slider::rotarySliderFillColourId, familyColour (v > 0.5f));
            repaint();
        });
        engine.onChange = [this] (int i) { engineAttachment->setValueAsCompleteGesture (i == 1 ? 1.0f : 0.0f); };
        engineAttachment->sendInitialUpdate();
    }
}

SynthPanel::~SynthPanel() = default;

void SynthPanel::refresh (int presetIndex, float peak)
{
    if (preset.getSelectedId() != presetIndex + 1) preset.setSelectedId (presetIndex + 1, juce::dontSendNotification);
    const float m = juce::jlimit (0.0f, 1.2f, peak);
    if (std::abs (m - meter) > 0.003f) { meter = m; repaint (meterArea); }
}

void SynthPanel::paint (juce::Graphics& g)
{
    g.setColour (colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
    drawCaption (g, titleArea, "Synth");
    // vertical peak meter (dBFS scale -48..+3)
    auto m = meterArea.toFloat();
    g.setColour (colours::bg);
    g.fillRoundedRectangle (m, 3.0f);
    const float db = meter > 0.0f ? 20.0f * std::log10 (meter) : -100.0f;
    const float frac = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 51.0f);
    auto fill = m.withTrimmedTop (m.getHeight() * (1.0f - frac));
    g.setColour (db > -1.0f ? colours::danger : (db > -9.0f ? colours::forge : colours::acid.withAlpha (0.9f)));
    g.fillRoundedRectangle (fill, 3.0f);
    g.setColour (colours::textDim);
    g.setFont (font (9.0f, true));
    g.drawText ("OUT", meterArea.withY (meterArea.getBottom() + 2).withHeight (12).expanded (6, 0), juce::Justification::centred);
}

void SynthPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    auto left = r.removeFromLeft (juce::jlimit (150, 200, r.getWidth() / 6));
    titleArea = left.removeFromTop (16);
    left.removeFromTop (4);
    engine.setBounds (left.removeFromTop (30));
    left.removeFromTop (8);
    preset.setBounds (left.removeFromTop (30));
    meterArea = r.removeFromRight (14).withTrimmedBottom (14).withTrimmedTop (4);
    r.removeFromRight (8);
    const int n = (int) knobs.size();
    const int w = r.getWidth() / n;
    for (int i = 0; i < n; ++i)
    {
        auto cell = r.removeFromLeft (w).reduced (3, 0);
        knobs[(size_t) i]->name.setBounds (cell.removeFromTop (16));
        knobs[(size_t) i]->slider.setBounds (cell);
    }
}
} // namespace gfb::ui
