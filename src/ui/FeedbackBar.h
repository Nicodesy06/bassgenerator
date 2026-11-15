#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/DesignSystem.h"

namespace gfb
{
class GrooveForgeProcessor;
}

namespace gfb::ui
{
/** LIKE / DISLIKE rating of the current pattern + taste learning switch.
    Every rating is stored locally (TasteStore) and changes the next generations. */
class FeedbackBar : public juce::Component
{
public:
    explicit FeedbackBar (GrooveForgeProcessor& p);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void (const juce::String&, bool)> onMessage;

private:
    GrooveForgeProcessor& proc;
    juce::TextButton like { "LIKE" }, dislike { "DISLIKE" }, tasteBtn { "TASTE" };
    juce::Label info;
    juce::String ratedState;
};
} // namespace gfb::ui
