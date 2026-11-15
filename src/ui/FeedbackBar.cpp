#include "ui/FeedbackBar.h"

#include "PluginProcessor.h"

namespace gfb::ui
{
FeedbackBar::FeedbackBar (GrooveForgeProcessor& p) : proc (p)
{
    like.setTooltip ("LIKE: this bassline is good. GrooveForge learns its rhythm, intervals and note lengths and generates more like it (stored locally, shared by all instances).");
    dislike.setTooltip ("DISLIKE: not for me. GrooveForge learns to avoid this kind of line and immediately generates a new one (locks respected).");
    tasteBtn.setTooltip ("TASTE on: generation uses what you taught it with LIKE / DISLIKE. Off: factory generator (useful to compare). Saved with the project.");
    tasteBtn.setClickingTogglesState (true);
    like.onClick = [this] {
        if (proc.tasteStore().alreadyRated (proc.getPattern())) { if (onMessage) onMessage ("This pattern was already rated", false); return; }
        const bool ok = proc.rateCurrent (true);
        if (onMessage) onMessage (ok ? juce::String ("Liked: GrooveForge will generate more lines like this one") : "Could not save rating: " + proc.tasteStore().lastError(), ! ok);
        refresh();
    };
    dislike.onClick = [this] {
        if (proc.tasteStore().alreadyRated (proc.getPattern())) { proc.generate (RollMode::Seeds); return; }
        const bool ok = proc.rateCurrent (false);
        if (onMessage) onMessage (ok ? juce::String ("Disliked: noted, here is a new line") : "Could not save rating: " + proc.tasteStore().lastError(), ! ok);
        refresh();
    };
    tasteBtn.onClick = [this] { proc.setUseTaste (tasteBtn.getToggleState()); };
    info.setFont (font (12.0f));
    info.setColour (juce::Label::textColourId, colours::textDim);
    for (auto* c : std::initializer_list<juce::Component*> { &like, &dislike, &tasteBtn, &info }) addAndMakeVisible (*c);
    refresh();
}

void FeedbackBar::refresh()
{
    const auto t = proc.tasteStore().profile();
    const int n = t.likes + t.dislikes;
    const bool rated = proc.tasteStore().alreadyRated (proc.getPattern());
    tasteBtn.setToggleState (proc.isUsingTaste(), juce::dontSendNotification);
    like.setEnabled (! rated);
    juce::String txt = n == 0 ? juce::String ("Rate patterns to teach GrooveForge your taste")
                              : "Learning from " + juce::String (n) + (n == 1 ? " rating" : " ratings") + "  (" + juce::String (t.likes) + " liked)";
    if (rated) txt = "Rated.  " + txt;
    if (! proc.isUsingTaste()) txt += "  -  taste off: factory generator";
    if (info.getText() != txt) info.setText (txt, juce::dontSendNotification);
}

void FeedbackBar::paint (juce::Graphics&) {}

void FeedbackBar::resized()
{
    auto r = getLocalBounds().reduced (0, 4);
    like.setBounds (r.removeFromLeft (84));
    r.removeFromLeft (6);
    dislike.setBounds (r.removeFromLeft (96));
    tasteBtn.setBounds (r.removeFromRight (80));
    r.removeFromLeft (14);
    info.setBounds (r);
}
} // namespace gfb::ui
