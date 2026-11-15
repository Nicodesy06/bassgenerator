#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "ui/BrowserPanel.h"
#include "ui/DesignSystem.h"
#include "ui/FeedbackBar.h"
#include "ui/GeneratorPanel.h"
#include "ui/PianoRoll.h"
#include "ui/SynthPanel.h"

namespace gfb
{
class GrooveForgeEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GrooveForgeEditor (GrooveForgeProcessor&);
    ~GrooveForgeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void showMessage (const juce::String& text, bool error);
    // exposed for UI binding tests
    ui::PianoRoll& getPianoRoll() noexcept { return roll; }

private:
    void timerCallback() override;
    void refreshFromProcessor();
    void startExternalDrag();
    void exportWithChooser();
    void addFavorite();
    void setDarkTheme (bool dark);

    GrooveForgeProcessor& proc;
    // applies the saved theme before the look-and-feel and the panels read the palette
    struct ThemeInit { ThemeInit() { ui::applyTheme (ui::loadDarkThemePreference()); } } themeInit;
    ui::ForgeLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };
    ui::GeneratorPanel generator;
    ui::PianoRoll roll;
    ui::FeedbackBar feedback;
    ui::ActionBar actions;
    ui::SynthPanel synthPanel;
    ui::BrowserPanel browser;
    juce::Label status, transport;
    juce::TextButton themeBtn { "DARK" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File lastExportDir;
    uint32_t seenVersion = 0;
    int favTick = 0;
    double messageUntil = 0;
    bool drawerOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveForgeEditor)
};
} // namespace gfb
