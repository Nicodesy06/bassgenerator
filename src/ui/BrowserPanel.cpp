#include "ui/BrowserPanel.h"

#include "PluginProcessor.h"
#include "core/ScaleTheory.h"

namespace gfb::ui
{
juce::String describePattern (const Pattern& p)
{
    const auto& s = p.settings;
    return juce::String (theory::rootName (s.root)) + " " + juce::String (std::string (scaleDisplayName (s.scale))) + "  " + juce::String (std::string (toString (s.family)))
         + " " + juce::String (std::string (toString (p.resolvedFeel))).toLowerCase() + "  " + juce::String (p.bars) + " bar  S" + juce::String (seedToString (s.styleSeed))
         + " V" + juce::String (seedToString (s.varSeed));
}

class BrowserPanel::HistoryModel : public juce::ListBoxModel
{
public:
    explicit HistoryModel (BrowserPanel& o) : owner (o) {}
    int getNumRows() override { return (int) owner.last.history.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= getNumRows()) return;
        const int idx = getNumRows() - 1 - row; // newest first
        const auto& p = owner.last.history[(size_t) idx];
        const bool current = idx == owner.last.historyCursor;
        g.setColour (selected ? colours::raised : (current ? colours::panel2 : colours::panel));
        g.fillRect (0, 0, w, h);
        g.setColour (familyColour (p.settings.family == Family::Acid));
        g.fillRect (0, 2, 3, h - 4);
        g.setColour (current ? colours::forge : colours::textDim);
        g.setFont (font (11.0f, true));
        g.drawText ("#" + juce::String (idx + 1), 8, 0, 34, h, juce::Justification::centredLeft);
        g.setColour (colours::text);
        g.setFont (font (12.0f));
        g.drawText (describePattern (p), 42, 0, w - 46, h, juce::Justification::centredLeft, true);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent&) override { select (row); }
    void returnKeyPressed (int row) override { select (row); }
    void select (int row) { owner.proc.historySelect (getNumRows() - 1 - row); }
    BrowserPanel& owner;
};

class BrowserPanel::FavoritesModel : public juce::ListBoxModel
{
public:
    explicit FavoritesModel (BrowserPanel& o) : owner (o) {}
    int getNumRows() override { return (int) owner.favs.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= getNumRows()) return;
        const auto& f = owner.favs[(size_t) row];
        g.setColour (selected ? colours::raised : colours::panel);
        g.fillRect (0, 0, w, h);
        g.setColour (familyColour (f.pattern.settings.family == Family::Acid));
        g.fillRect (0, 2, 3, h - 4);
        g.setColour (colours::text);
        g.setFont (font (13.0f, true));
        g.drawText (f.name, 8, 2, w - 12, h / 2, juce::Justification::centredLeft, true);
        g.setColour (colours::textDim);
        g.setFont (font (11.0f));
        g.drawText (describePattern (f.pattern), 8, h / 2, w - 12, h / 2 - 2, juce::Justification::centredLeft, true);
    }
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { load (row); }
    void returnKeyPressed (int row) override { load (row); }
    void deleteKeyPressed (int) override { owner.favDelete.triggerClick(); }
    void load (int row)
    {
        if (row >= 0 && row < getNumRows()) owner.proc.loadPattern (owner.favs[(size_t) row].pattern);
    }
    BrowserPanel& owner;
};

BrowserPanel::BrowserPanel (GrooveForgeProcessor& p) : proc (p)
{
    historyModel = std::make_unique<HistoryModel> (*this);
    favModel = std::make_unique<FavoritesModel> (*this);
    historyList.setModel (historyModel.get());
    historyList.setRowHeight (24);
    historyList.setTooltip ("Last 64 generated patterns (exact snapshots, newest first). Click to recall notes + generator settings.");
    favList.setModel (favModel.get());
    favList.setRowHeight (38);
    favList.setTooltip ("Favorites are shared by all GrooveForge instances and sessions. Double-click to load.");
    historyPage.addAndMakeVisible (historyList);
    favPage.addAndMakeVisible (favList);
    for (auto* b : { &favLoad, &favRename, &favDelete, &favReveal }) favPage.addAndMakeVisible (*b);
    favLoad.onClick = [this] { favModel->load (favList.getSelectedRow()); };
    favDelete.onClick = [this] {
        const int row = favList.getSelectedRow();
        if (row < 0 || row >= (int) favs.size()) return;
        const auto id = favs[(size_t) row].id;
        if (! proc.favorites().remove (id) && onMessage) onMessage ("Could not delete favorite: " + proc.favorites().lastError(), true);
        refreshFavorites();
    };
    favRename.onClick = [this] {
        const int row = favList.getSelectedRow();
        if (row < 0 || row >= (int) favs.size()) return;
        auto* aw = new juce::AlertWindow ("Rename favorite", "New name:", juce::MessageBoxIconType::NoIcon, this);
        aw->addTextEditor ("name", favs[(size_t) row].name);
        aw->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        const auto id = favs[(size_t) row].id;
        juce::Component::SafePointer<BrowserPanel> safe (this);
        aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw, id] (int result) {
            if (safe == nullptr || result != 1) return;
            const auto name = aw->getTextEditorContents ("name").trim();
            if (name.isNotEmpty()) safe->proc.favorites().rename (id, name);
            safe->refreshFavorites();
        }), true);
    };
    favReveal.setTooltip ("Reveal favorites.json in the file browser (back it up / move it between machines)");
    favReveal.onClick = [this] {
        const auto f = proc.favorites().getFile();
        if (f.existsAsFile())
            f.revealToUser();
        else if (f.getParentDirectory().createDirectory())
            f.getParentDirectory().revealToUser();
    };

    // Advanced
    swing.setSliderStyle (juce::Slider::LinearHorizontal);
    swing.setRange (49, 66, 1);
    swing.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    swing.textFromValueFunction = [] (double v) { return v < 50 ? juce::String ("Auto") : juce::String ((int) v) + " %"; };
    swing.setTooltip ("Swing of the off-16ths. Auto = feel/archetype default (tight). Changing it regenerates with the same seeds.");
    swing.onValueChange = [this] {
        if (updating) return;
        auto s = proc.getStateSnapshot().settings;
        s.swing = swing.getValue() < 50 ? kAutoSwing : (int) swing.getValue();
        proc.setSettings (s);
    };
    kick.addItem ("Four on the floor", 1);
    kick.addItem ("Broken / offbeat kick", 2);
    kick.addItem ("Open (no kick assumption)", 3);
    kick.setTooltip ("Implied drum context: a prior for groove selection and scoring (not audio analysis)");
    kick.onChange = [this] {
        if (updating) return;
        auto s = proc.getStateSnapshot().settings;
        s.kickContext = (KickContext) juce::jmax (0, kick.getSelectedId() - 1);
        proc.setSettings (s);
    };
    previewBpm.setSliderStyle (juce::Slider::LinearHorizontal);
    previewBpm.setRange (60, 200, 0.5);
    previewBpm.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    previewBpm.setTooltip ("Tempo used for preview and MIDI export when the host does not provide one");
    previewBpm.onValueChange = [this] { if (! updating) proc.setPreviewBpm (previewBpm.getValue()); };
    swingLabel.setText ("Swing", juce::dontSendNotification);
    kickLabel.setText ("Rhythmic context", juce::dontSendNotification);
    bpmLabel.setText ("Fallback / preview BPM", juce::dontSendNotification);
    info.setText ("History lives in this instance and is saved with the project. Favorites are global: "
                  + proc.favorites().getFile().getFullPathName(), juce::dontSendNotification);
    info.setFont (font (11.0f));
    info.setColour (juce::Label::textColourId, colours::textDim);
    info.setJustificationType (juce::Justification::topLeft);
    resetBtn.setTooltip ("Reset generator settings, locks and synth parameters to defaults (seeds and history are kept)");
    resetBtn.onClick = [this] { proc.resetToDefaults(); };
    for (auto* c : std::initializer_list<juce::Component*> { &swing, &kick, &previewBpm, &swingLabel, &kickLabel, &bpmLabel, &info, &resetBtn })
        advPage.addAndMakeVisible (*c);

    tabs.addTab ("HISTORY", colours::panel, &historyPage, false);
    tabs.addTab ("FAVORITES", colours::panel, &favPage, false);
    tabs.addTab ("ADVANCED", colours::panel, &advPage, false);
    tabs.setTabBarDepth (30);
    tabs.setOutline (0);
    addAndMakeVisible (tabs);
    refreshFavorites();
}

BrowserPanel::~BrowserPanel()
{
    historyList.setModel (nullptr);
    favList.setModel (nullptr);
}

void BrowserPanel::refresh (const state::InstanceState& s)
{
    juce::ScopedValueSetter<bool> svs (updating, true);
    last = s;
    historyList.updateContent();
    historyList.repaint();
    swing.setValue (s.settings.swing == kAutoSwing ? 49 : s.settings.swing, juce::dontSendNotification);
    kick.setSelectedId ((int) s.settings.kickContext + 1, juce::dontSendNotification);
    previewBpm.setValue (s.previewBpm, juce::dontSendNotification);
}

void BrowserPanel::refreshFavorites()
{
    favs = proc.favorites().list();
    favList.updateContent();
    favList.repaint();
}

void BrowserPanel::paint (juce::Graphics& g)
{
    g.setColour (colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
}

void BrowserPanel::resized()
{
    tabs.setBounds (getLocalBounds().reduced (6));
    const auto page = tabs.getLocalBounds().withTrimmedTop (30);
    historyList.setBounds (page.withZeroOrigin().reduced (4));
    auto f = page.withZeroOrigin().reduced (4);
    auto buttons = f.removeFromBottom (30);
    const int bw = buttons.getWidth() / 4;
    for (auto* b : { &favLoad, &favRename, &favDelete, &favReveal }) b->setBounds (buttons.removeFromLeft (bw).reduced (2, 0));
    f.removeFromBottom (6);
    favList.setBounds (f);
    auto a = page.withZeroOrigin().reduced (10);
    swingLabel.setBounds (a.removeFromTop (20));
    swing.setBounds (a.removeFromTop (28));
    a.removeFromTop (8);
    kickLabel.setBounds (a.removeFromTop (20));
    kick.setBounds (a.removeFromTop (28));
    a.removeFromTop (8);
    bpmLabel.setBounds (a.removeFromTop (20));
    previewBpm.setBounds (a.removeFromTop (28));
    a.removeFromTop (12);
    resetBtn.setBounds (a.removeFromTop (30));
    a.removeFromTop (12);
    info.setBounds (a.removeFromTop (90));
}
} // namespace gfb::ui
