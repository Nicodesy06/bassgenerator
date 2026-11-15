#include "ui/SynthPanel.h"

#include "PluginProcessor.h"
#include "dsp/SynthParams.h"

namespace gfb::ui
{
namespace
{
struct KnobDef { const char* id; const char* label; const char* tip; };
const KnobDef kKnobs[] = {
    { "volume", "Volume", "Output level (smoothed)" },
    { "cutoff", "Cutoff", "Filter cutoff (log, 30 Hz - 12 kHz)" },
    { "resonance", "Resonance", "Filter resonance (bounded below self-oscillation)" },
    { "envmod", "Env mod", "Filter envelope depth" },
    { "decay", "Decay", "Filter / amp envelope decay time" },
    { "accent", "Accent", "How strongly accented notes hit: level, filter sweep, drive" },
    { "drive", "Drive", "Saturation (2x oversampled, level compensated)" },
    { "glide", "Glide", "Slide (portamento) time for notes flagged slide" },
    { "shape", "Shape", "Oscillator shape: SUB harmonic layer amount / saw-square; ACID saw-square" },
};
} // namespace

SynthPanel::SynthPanel (GrooveForgeProcessor& p) : proc (p)
{
    for (const auto& d : kKnobs)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::LinearHorizontal);
        k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0); // value drawn by ForgeLookAndFeel
        k->slider.setTooltip (juce::String (d.tip) + ". Double-click: default. Shift-drag: fine.");
        k->slider.setWantsKeyboardFocus (true);
        k->slider.setDoubleClickReturnValue (true, 0.0); // replaced by attachment default below
        k->slider.setVelocityModeParameters (1.0, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
        k->name.setText (d.label, juce::dontSendNotification);
        k->name.setJustificationType (juce::Justification::centredLeft);
        k->name.setFont (font (11.5f));
        k->name.setColour (juce::Label::textColourId, colours::textDim);
        k->name.setInterceptsMouseClicks (false, false);
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
            for (auto& k : knobs) k->slider.setColour (juce::Slider::trackColourId, familyColour (v > 0.5f));
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
    g.setColour (colours::ink);
    g.fillRect (0, 0, getWidth(), 2);
    g.setFont (font (12.0f, true));
    g.drawText ("Synth", titleArea, juce::Justification::centredLeft);
    // peak meter: one ink bar on a hairline, red above -1 dBFS
    auto m = meterArea.toFloat();
    g.setColour (colours::rule);
    g.fillRect (m.getCentreX() - 0.5f, m.getY(), 1.0f, m.getHeight());
    const float db = meter > 0.0f ? 20.0f * std::log10 (meter) : -100.0f;
    const float frac = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 51.0f);
    auto fill = m.withTrimmedTop (m.getHeight() * (1.0f - frac));
    g.setColour (db > -1.0f ? colours::red : colours::ink);
    g.fillRect (fill.reduced (2.0f, 0.0f));
    g.setColour (colours::textDim);
    g.setFont (font (10.0f));
    g.drawText ("Out", meterArea.withY (meterArea.getBottom() + 2).withHeight (12).expanded (8, 0), juce::Justification::centred);
}

void SynthPanel::resized()
{
    auto r = getLocalBounds().withTrimmedTop (8);
    auto left = r.removeFromLeft (juce::jlimit (160, 220, r.getWidth() / 6));
    titleArea = left.removeFromTop (18);
    left.removeFromTop (6);
    left.removeFromRight (16);
    engine.setBounds (left.removeFromTop (28));
    left.removeFromTop (10);
    preset.setBounds (left.removeFromTop (28));
    meterArea = r.removeFromRight (14).withTrimmedBottom (16).withTrimmedTop (6);
    r.removeFromRight (16);
    // 3 x 3 grid of linear sliders
    const int cols = 3, rows = 3, gx = 24;
    const int cw = (r.getWidth() - gx * (cols - 1)) / cols;
    const int rh = r.getHeight() / rows;
    for (int i = 0; i < (int) knobs.size(); ++i)
    {
        const int c = i % cols, row = i / cols;
        auto cell = juce::Rectangle<int> (r.getX() + c * (cw + gx), r.getY() + row * rh, cw, rh).reduced (0, 2);
        knobs[(size_t) i]->name.setBounds (cell.withHeight (juce::jmin (16, cell.getHeight() / 2)));
        knobs[(size_t) i]->slider.setBounds (cell);
    }
}
} // namespace gfb::ui
