#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "ui/DesignSystem.h"

namespace gfb
{
class GrooveForgeProcessor;
}

namespace gfb::ui
{
class SynthPanel : public juce::Component
{
public:
    explicit SynthPanel (GrooveForgeProcessor& p);
    ~SynthPanel() override;
    void refresh (int presetIndex, float peak);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    GrooveForgeProcessor& proc;
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ComboBox preset;
    Segmented engine { { "SUB", "ACID" } };
    std::unique_ptr<juce::ParameterAttachment> engineAttachment;
    float meter = 0.0f;
    juce::Rectangle<int> meterArea, titleArea;
};
} // namespace gfb::ui
