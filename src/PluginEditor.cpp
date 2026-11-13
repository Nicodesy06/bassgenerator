#include "PluginEditor.h"

#include "midi/MidiFileExport.h"

namespace gfb
{
GrooveForgeEditor::GrooveForgeEditor (GrooveForgeProcessor& p)
    : AudioProcessorEditor (p), proc (p), generator (p), actions (p), synthPanel (p), browser (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (generator);
    addAndMakeVisible (roll);
    addAndMakeVisible (actions);
    addAndMakeVisible (synthPanel);
    addChildComponent (browser);
    for (auto* l : { &status, &transport })
    {
        l->setFont (ui::font (12.0f, true));
        l->setColour (juce::Label::textColourId, ui::colours::textDim);
        addAndMakeVisible (*l);
    }
    transport.setJustificationType (juce::Justification::centredRight);

    roll.onExternalDrag = [this] { startExternalDrag(); };
    actions.onDragStart = [this] { startExternalDrag(); };
    actions.onExport = [this] { exportWithChooser(); };
    actions.onAddFavorite = [this] { addFavorite(); };
    actions.onToggleBrowser = [this] {
        drawerOpen = ! drawerOpen;
        proc.setDrawerOpen (drawerOpen);
        browser.setVisible (drawerOpen);
        resized();
    };
    browser.onMessage = [this] (const juce::String& t, bool e) { showMessage (t, e); };
    lastExportDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

    const auto s = proc.getStateSnapshot();
    drawerOpen = s.drawerOpen;
    browser.setVisible (drawerOpen);
    setResizable (true, true);
    setResizeLimits (1100, 640, 2600, 1700);
    setSize (s.editorWidth, s.editorHeight);
    setWantsKeyboardFocus (true);
    refreshFromProcessor();
    showMessage ("G generate  S new style  V new var  F favorite  Ctrl+Z/Y history  -  drag the roll into your DAW", false);
    startTimerHz (30);
}

GrooveForgeEditor::~GrooveForgeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void GrooveForgeEditor::paint (juce::Graphics& g) { g.fillAll (ui::colours::bg); }

void GrooveForgeEditor::resized()
{
    auto r = getLocalBounds().reduced (8);
    const float h = (float) r.getHeight();
    generator.setBounds (r.removeFromTop (juce::jlimit (74, 96, (int) (h * 0.11f))));
    r.removeFromTop (6);
    auto foot = r.removeFromBottom (18);
    transport.setBounds (foot.removeFromRight (360));
    status.setBounds (foot);
    r.removeFromBottom (4);
    synthPanel.setBounds (r.removeFromBottom (juce::jlimit (130, 190, (int) (h * 0.22f))));
    r.removeFromBottom (6);
    actions.setBounds (r.removeFromBottom (46));
    r.removeFromBottom (6);
    if (drawerOpen)
    {
        browser.setBounds (r.removeFromRight (juce::jlimit (300, 420, r.getWidth() / 3)));
        r.removeFromRight (6);
    }
    roll.setBounds (r);
    proc.setEditorSize (getWidth(), getHeight());
}

void GrooveForgeEditor::refreshFromProcessor()
{
    const auto s = proc.getStateSnapshot();
    generator.refresh (s);
    actions.refresh (s, proc.isPreviewOn(), s.hostSync, s.midiOut);
    actions.browserBtn.setToggleState (drawerOpen, juce::dontSendNotification);
    roll.setPattern (s.pattern);
    roll.setAccentColour (ui::familyColour (s.settings.family == Family::Acid));
    browser.refresh (s);
    synthPanel.refresh (s.presetIndex, proc.uiPeak.load());
}

void GrooveForgeEditor::timerCallback()
{
    const auto v = proc.stateVersion.load();
    if (v != seenVersion)
    {
        seenVersion = v;
        refreshFromProcessor();
    }
    roll.setPlayheadTick (proc.uiPlayheadTick.load());
    synthPanel.refresh (proc.getCurrentProgram(), proc.uiPeak.load());
    if (++favTick >= 30)
    {
        favTick = 0;
        if (proc.favorites().reloadIfChanged()) browser.refreshFavorites(); // another instance changed favorites
    }
    const double bpm = proc.uiHostBpm.load();
    juce::String t;
    if (proc.uiHostPlaying.load() && proc.getStateSnapshot().hostSync) t = "HOST PLAY  " + juce::String (bpm, 1) + " BPM";
    else if (proc.isPreviewOn()) t = "PREVIEW  " + juce::String (bpm > 0 ? bpm : proc.getStateSnapshot().previewBpm, 1) + " BPM" + (bpm > 0 ? "" : " (fallback)");
    else t = "STOPPED" + (bpm > 0 ? "  host " + juce::String (bpm, 1) + " BPM" : juce::String ("  no host tempo: export uses ") + juce::String (proc.exportBpm(), 1) + " BPM");
    if (transport.getText() != t) transport.setText (t, juce::dontSendNotification);
    if (messageUntil > 0 && juce::Time::getMillisecondCounterHiRes() > messageUntil)
    {
        messageUntil = 0;
        status.setText ({}, juce::dontSendNotification);
    }
}

void GrooveForgeEditor::showMessage (const juce::String& text, bool error)
{
    status.setColour (juce::Label::textColourId, error ? ui::colours::danger : ui::colours::textDim);
    status.setText (text, juce::dontSendNotification);
    messageUntil = juce::Time::getMillisecondCounterHiRes() + (error ? 8000.0 : 5000.0);
}

void GrooveForgeEditor::startExternalDrag()
{
    juce::File f;
    const auto r = proc.prepareDragFile (f);
    if (r.failed()) { showMessage ("Drag failed: " + r.getErrorMessage() + "  Use EXPORT MIDI instead.", true); return; }
    juce::Component::SafePointer<GrooveForgeEditor> safe (this);
    const bool started = juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, &roll, [safe] {
        if (safe != nullptr) safe->showMessage ("MIDI clip dropped (file kept in the GrooveForge cache folder)", false);
    });
    if (! started) showMessage ("This host/window did not accept an external drag. Use EXPORT MIDI. File: " + f.getFullPathName(), true);
}

void GrooveForgeEditor::exportWithChooser()
{
    const auto p = proc.getPattern();
    chooser = std::make_unique<juce::FileChooser> ("Export MIDI", lastExportDir.getChildFile (juce::String (midi::suggestedFileName (p))), "*.mid");
    juce::Component::SafePointer<GrooveForgeEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc) {
                              if (safe == nullptr) return;
                              auto file = fc.getResult();
                              if (file == juce::File()) return;
                              if (! file.hasFileExtension ("mid")) file = file.withFileExtension ("mid");
                              const auto r = safe->proc.exportMidi (file);
                              if (r.wasOk())
                              {
                                  safe->lastExportDir = file.getParentDirectory();
                                  safe->showMessage ("Saved " + file.getFullPathName(), false);
                              }
                              else safe->showMessage ("Export failed: " + r.getErrorMessage(), true);
                          });
}

void GrooveForgeEditor::addFavorite()
{
    const auto p = proc.getPattern();
    const auto name = ui::describePattern (p).upToFirstOccurrenceOf ("  S", false, false);
    if (auto id = proc.favorites().add (p, name))
    {
        browser.refreshFavorites();
        showMessage ("Added to favorites: " + name, false);
    }
    else showMessage ("Could not save favorite: " + proc.favorites().lastError(), true);
}

bool GrooveForgeEditor::keyPressed (const juce::KeyPress& k)
{
    const auto c = juce::CharacterFunctions::toLowerCase (k.getTextCharacter());
    if (k.getModifiers().isCommandDown())
    {
        if (k.getKeyCode() == 'Z' || c == 'z') { proc.historyBack(); return true; }
        if (k.getKeyCode() == 'Y' || c == 'y') { proc.historyForward(); return true; }
        return false;
    }
    if (c == 'g') { proc.generate (RollMode::Seeds); return true; }
    if (c == 's') { proc.generate (RollMode::StyleOnly); return true; }
    if (c == 'v') { proc.generate (RollMode::VarOnly); return true; }
    if (c == 'r') { proc.generate (RollMode::Everything); return true; }
    if (c == 'f') { addFavorite(); return true; }
    if (c == 'p') { proc.setPreview (! proc.isPreviewOn()); return true; }
    if (k == juce::KeyPress::leftKey) { proc.historyBack(); return true; }
    if (k == juce::KeyPress::rightKey) { proc.historyForward(); return true; }
    return false;
}
} // namespace gfb
