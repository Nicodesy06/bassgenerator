#include "ui/GeneratorPanel.h"

#include "PluginProcessor.h"
#include "core/ScaleTheory.h"

namespace gfb::ui
{
namespace
{
void styleCombo (juce::ComboBox& c, const juce::String& tip)
{
    c.setTooltip (tip);
    c.setWantsKeyboardFocus (true);
}
const char* kLockTip = "Lock: RANDOMISE (and NEW STYLE / NEW VAR for seeds) will never change a locked setting. Locks are saved with the project.";
} // namespace

GeneratorPanel::GeneratorPanel (GrooveForgeProcessor& p) : proc (p)
{
    for (int i = 0; i < 12; ++i) root.addItem (juce::String (theory::rootName (i)), i + 1);
    for (int i = 0; i < kNumScales; ++i) scale.addItem (juce::String (std::string (scaleDisplayName ((ScaleType) i))), i + 1);
    const char* feels[] = { "Random", "Sparse", "Balanced", "Busy", "Rolling", "Syncopated", "Stabby" };
    for (int i = 0; i < kNumFeels; ++i) feel.addItem (feels[i], i + 1);
    styleCombo (root, "Root note (key centre)");
    styleCombo (scale, "Scale / mode. Generated notes never leave it.");
    styleCombo (feel, "Feel: rhythmic character. Random picks a weighted concrete feel from the STYLE seed.");
    family.setTooltip ("Family: SUB = round club sub lines (root/fifth/octave, longer notes). ACID = accented, sliding, step-wise hooks. Also switches the synth engine.");
    bars.setTooltip ("Phrase length in 4/4 bars");
    for (auto* c : { &root, &scale, &feel }) { addAndMakeVisible (*c); c->onChange = [this] { pushSettings(); }; }
    complexity.setSliderStyle (juce::Slider::LinearHorizontal);
    complexity.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    complexity.setRange (0, 100, 5);
    complexity.setDoubleClickReturnValue (true, kNeutralComplexity);
    complexity.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; };
    complexity.setTooltip ("Complexity: how many notes the line has, inside the chosen Feel. 50 % = the feel's natural density, higher = more notes and movement, lower = fewer notes and more space. Double-click: 50 %.");
    complexity.setWantsKeyboardFocus (true);
    // regenerate once per gesture (dragging would flood the history)
    complexity.onDragEnd = [this] { pushSettings(); };
    complexity.onValueChange = [this] { if (! complexity.isMouseButtonDown()) pushSettings(); };
    addAndMakeVisible (complexity);
    addAndMakeVisible (family);
    addAndMakeVisible (bars);
    family.onChange = [this] (int) { pushSettings(); };
    bars.onChange = [this] (int) { pushSettings(); };
    octLabel.setJustificationType (juce::Justification::centred);
    octLabel.setFont (font (15.0f, true));
    octLabel.setTooltip ("Octave transposition (-3..+3). Notes stay valid MIDI: the whole line folds by octaves at the extremes.");
    addAndMakeVisible (octLabel);
    for (auto* b : { &octDown, &octUp })
    {
        addAndMakeVisible (*b);
        b->setWantsKeyboardFocus (true);
    }
    octDown.onClick = [this] { if (current.octave > kMinOctave) { current.octave--; pushSettings(); } };
    octUp.onClick = [this] { if (current.octave < kMaxOctave) { current.octave++; pushSettings(); } };
    for (auto* l : { &lRoot, &lScale, &lFamily, &lFeel, &lBars, &lOctave })
    {
        addAndMakeVisible (*l);
        l->setTooltip (kLockTip);
        l->onClick = [this] { pushLocks(); };
    }
    generateBtn.setColour (juce::TextButton::buttonColourId, colours::ink);
    generateBtn.setTooltip ("Generate a new bassline: new STYLE + VAR seeds (unless locked), current key/scale/family/feel/bars. Shortcut: G");
    generateBtn.onClick = [this] { proc.generate (RollMode::Seeds); };
    diceBtn.setTooltip ("Randomise every unlocked setting (key, scale, family, feel, bars, octave) and both seeds");
    diceBtn.onClick = [this] { proc.generate (RollMode::Everything); };
    addAndMakeVisible (generateBtn);
    addAndMakeVisible (diceBtn);
}

void GeneratorPanel::refresh (const state::InstanceState& s)
{
    juce::ScopedValueSetter<bool> svs (updating, true);
    current = s.settings;
    root.setSelectedId (s.settings.root + 1, juce::dontSendNotification);
    scale.setSelectedId ((int) s.settings.scale + 1, juce::dontSendNotification);
    feel.setSelectedId ((int) s.settings.feel + 1, juce::dontSendNotification);
    complexity.setValue (s.settings.complexity, juce::dontSendNotification);
    family.setSelected ((int) s.settings.family, false);
    family.setAccent (familyColour (s.settings.family == Family::Acid));
    const int bi = s.settings.bars == 1 ? 0 : s.settings.bars == 2 ? 1 : s.settings.bars == 4 ? 2 : 3;
    bars.setSelected (bi, false);
    octLabel.setText ((s.settings.octave > 0 ? "+" : "") + juce::String (s.settings.octave), juce::dontSendNotification);
    lRoot.setToggleState (s.locks.root, juce::dontSendNotification);
    lScale.setToggleState (s.locks.scale, juce::dontSendNotification);
    lFamily.setToggleState (s.locks.family, juce::dontSendNotification);
    lFeel.setToggleState (s.locks.feel, juce::dontSendNotification);
    lBars.setToggleState (s.locks.bars, juce::dontSendNotification);
    lOctave.setToggleState (s.locks.octave, juce::dontSendNotification);
    octDown.setEnabled (s.settings.octave > kMinOctave);
    octUp.setEnabled (s.settings.octave < kMaxOctave);
}

void GeneratorPanel::pushSettings()
{
    if (updating) return;
    auto s = current;
    s.root = juce::jmax (0, root.getSelectedId() - 1);
    s.scale = (ScaleType) juce::jmax (0, scale.getSelectedId() - 1);
    s.feel = (Feel) juce::jmax (0, feel.getSelectedId() - 1);
    s.complexity = juce::roundToInt (complexity.getValue());
    s.family = (Family) family.getSelected();
    const int barsOpts[] = { 1, 2, 4, 8 };
    s.bars = barsOpts[bars.getSelected()];
    proc.setSettings (s);
}

void GeneratorPanel::pushLocks()
{
    if (updating) return;
    auto l = proc.getStateSnapshot().locks;
    l.root = lRoot.getToggleState(); l.scale = lScale.getToggleState(); l.family = lFamily.getToggleState();
    l.feel = lFeel.getToggleState(); l.bars = lBars.getToggleState(); l.octave = lOctave.getToggleState();
    proc.setLocks (l);
}

void GeneratorPanel::paint (juce::Graphics& g)
{
    // Swiss masthead: heavy rule, bold wordmark, small red square as the only ornament
    g.setColour (colours::ink);
    g.fillRect (0, 0, getWidth(), 3);
    auto logo = getLocalBounds().withTrimmedTop (10).removeFromLeft (logoWidth - 8);
    g.setFont (font (juce::jlimit (18.0f, 26.0f, (float) logoWidth * 0.14f), true));
    g.drawText ("GrooveForge", logo.removeFromTop (logo.getHeight() / 2 + 4), juce::Justification::bottomLeft);
    auto sub = logo.withTrimmedTop (2);
    g.setColour (colours::red);
    g.fillRect (sub.getX(), sub.getY() + 4, 9, 9);
    g.setColour (colours::textDim);
    g.setFont (font (12.5f));
    g.drawText ("Bass  " GFB_VERSION_STRING, sub.withTrimmedLeft (15).withHeight (17), juce::Justification::centredLeft);
    for (auto& [r, t] : captions) drawCaption (g, r, t);
}

void GeneratorPanel::resized()
{
    captions.clear();
    auto r = getLocalBounds().withTrimmedTop (10).withTrimmedBottom (4);
    logoWidth = juce::jlimit (128, 186, getWidth() / 8);
    r.removeFromLeft (logoWidth);
    auto right = r.removeFromRight (juce::jmin (280, r.getWidth() / 5));
    diceBtn.setBounds (right.removeFromRight (right.getWidth() * 2 / 5).reduced (4, 10));
    generateBtn.setBounds (right.reduced (4, 6));
    const int lockW = 20;
    struct G { juce::String cap; float weight; };
    const G groups[] = { { "Key", 1.0f }, { "Scale", 1.7f }, { "Family", 1.55f }, { "Feel", 1.5f }, { "Complexity", 1.4f }, { "Bars", 1.45f }, { "Octave", 1.15f } };
    float total = 0;
    for (auto& g : groups) total += g.weight;
    const int gap = 10;
    const int avail = r.getWidth() - gap * 6;
    int x = r.getX();
    for (int i = 0; i < 7; ++i)
    {
        const int w = (int) ((float) avail * groups[i].weight / total);
        auto cell = juce::Rectangle<int> (x, r.getY(), w, r.getHeight());
        x += w + gap;
        captions.push_back ({ cell.removeFromTop (16).translated (0, 2), groups[i].cap });
        auto row = cell.withSizeKeepingCentre (cell.getWidth(), juce::jmin (34, cell.getHeight()));
        auto lockR = row.removeFromRight (lockW);
        row.removeFromRight (4);
        switch (i)
        {
            case 0: root.setBounds (row); lRoot.setBounds (lockR); break;
            case 1: scale.setBounds (row); lScale.setBounds (lockR); break;
            case 2: family.setBounds (row); lFamily.setBounds (lockR); break;
            case 3: feel.setBounds (row); lFeel.setBounds (lockR); break;
            case 4: complexity.setBounds (row.withTrimmedRight (-lockW - 4)); break; // no lock
            case 5: bars.setBounds (row); lBars.setBounds (lockR); break;
            case 6:
            {
                const int bw = juce::jmin (30, row.getWidth() / 3);
                octDown.setBounds (row.removeFromLeft (bw));
                octUp.setBounds (row.removeFromRight (bw));
                octLabel.setBounds (row);
                lOctave.setBounds (lockR);
                break;
            }
            default: break;
        }
    }
}

// ============================================================================ ActionBar
ActionBar::ActionBar (GrooveForgeProcessor& p) : proc (p)
{
    for (auto* c : std::initializer_list<juce::Component*> { &style, &var, &lStyle, &lVar, &newStyle, &newVar, &back, &fwd, &histLabel, &favBtn, &exportBtn,
                                                              &dragBtn, &previewBtn, &syncBtn, &midiOutBtn, &browserBtn })
        addAndMakeVisible (*c);
    style.setAccent (colours::ink);
    var.setAccent (colours::ink);
    auto pushSeeds = [this] {
        if (updating) return;
        auto s = proc.getStateSnapshot().settings;
        s.styleSeed = style.getValue();
        s.varSeed = var.getValue();
        proc.setSettings (s);
    };
    style.onChange = [pushSeeds] (uint64_t) { pushSeeds(); };
    var.onChange = [pushSeeds] (uint64_t) { pushSeeds(); };
    lStyle.setTooltip ("Lock STYLE: GENERATE/RANDOMISE keep this seed (only VAR changes => variations of the same groove idea)");
    lVar.setTooltip ("Lock VAR: GENERATE/RANDOMISE keep this seed");
    auto pushLocks = [this] {
        if (updating) return;
        auto l = proc.getStateSnapshot().locks;
        l.styleSeed = lStyle.getToggleState();
        l.varSeed = lVar.getToggleState();
        proc.setLocks (l);
    };
    lStyle.onClick = pushLocks;
    lVar.onClick = pushLocks;
    newStyle.setTooltip ("New STYLE seed only: new archetype, motif and phrase design (shortcut S)");
    newVar.setTooltip ("New VAR seed only: bounded variation of the same idea - fills, answers, small pitch/rhythm changes (shortcut V)");
    newStyle.onClick = [this] { proc.generate (RollMode::StyleOnly); };
    newVar.onClick = [this] { proc.generate (RollMode::VarOnly); };
    back.setTooltip ("Previous pattern in history (Ctrl+Z / Left)");
    fwd.setTooltip ("Next pattern in history (Ctrl+Y / Right)");
    back.onClick = [this] { proc.historyBack(); };
    fwd.onClick = [this] { proc.historyForward(); };
    histLabel.setJustificationType (juce::Justification::centred);
    histLabel.setFont (font (12.0f, true));
    histLabel.setColour (juce::Label::textColourId, colours::textDim);
    favBtn.setTooltip ("Save this exact pattern (notes + settings + seeds) to the global Favorites shared by all instances (shortcut F)");
    favBtn.onClick = [this] { if (onAddFavorite) onAddFavorite(); };
    exportBtn.setTooltip ("Save the pattern as a Standard MIDI File (.mid, PPQ 960, tempo + 4/4 included)");
    exportBtn.onClick = [this] { if (onExport) onExport(); };
    dragBtn.setTooltip ("Drag this button (or the piano roll) onto a DAW track to drop the pattern as a MIDI clip");
    dragBtn.setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    dragHandle.start = [this] { if (onDragStart) onDragStart(); };
    dragBtn.addMouseListener (&dragHandle, false);
    previewBtn.setClickingTogglesState (true);
    previewBtn.setTooltip ("Preview: loop the pattern with the internal synth while the host transport is stopped (uses host tempo if known, else the preview BPM in LIBRARY > Advanced)");
    previewBtn.onClick = [this] { proc.setPreview (previewBtn.getToggleState()); };
    syncBtn.setClickingTogglesState (true);
    syncBtn.setTooltip ("Host Sync: when the DAW plays, the pattern follows the host song position and tempo");
    syncBtn.onClick = [this] { proc.setHostSync (syncBtn.getToggleState()); };
    midiOutBtn.setClickingTogglesState (true);
    midiOutBtn.setTooltip ("MIDI Out: also send the generated notes as VST3 MIDI output (host routing dependent; .mid export always works)");
    midiOutBtn.onClick = [this] { proc.setMidiOut (midiOutBtn.getToggleState()); };
    browserBtn.setClickingTogglesState (true);
    browserBtn.setTooltip ("Show History (last 64), Favorites and Advanced groove options");
    browserBtn.onClick = [this] { if (onToggleBrowser) onToggleBrowser(); };
}

void ActionBar::refresh (const state::InstanceState& s, bool preview, bool sync, bool midiOut)
{
    juce::ScopedValueSetter<bool> svs (updating, true);
    style.setValue (s.settings.styleSeed, false);
    var.setValue (s.settings.varSeed, false);
    lStyle.setToggleState (s.locks.styleSeed, juce::dontSendNotification);
    lVar.setToggleState (s.locks.varSeed, juce::dontSendNotification);
    newStyle.setEnabled (! s.locks.styleSeed);
    newVar.setEnabled (! s.locks.varSeed);
    const int n = (int) s.history.size();
    histLabel.setText (n == 0 ? "-" : juce::String (s.historyCursor + 1) + " / " + juce::String (n), juce::dontSendNotification);
    back.setEnabled (s.historyCursor > 0);
    fwd.setEnabled (s.historyCursor + 1 < n);
    previewBtn.setToggleState (preview, juce::dontSendNotification);
    syncBtn.setToggleState (sync, juce::dontSendNotification);
    midiOutBtn.setToggleState (midiOut, juce::dontSendNotification);
}

void ActionBar::paint (juce::Graphics& g)
{
    g.setColour (colours::rule);
    g.fillRect (0, 0, getWidth(), 1);
}

void ActionBar::resized()
{
    auto r = getLocalBounds().withTrimmedTop (8).withTrimmedBottom (2);
    const int W = r.getWidth();
    // proportional layout: seeds | history | actions, every element keeps a readable minimum
    auto take = [&] (int w) { auto a = r.removeFromLeft (w); r.removeFromLeft (5); return a; };
    const int seedW = juce::jlimit (128, 210, W * 15 / 100);
    const int btnW = juce::jlimit (60, 96, W * 75 / 1000);
    style.setBounds (take (seedW));
    lStyle.setBounds (take (16));
    newStyle.setBounds (take (btnW + 6));
    r.removeFromLeft (4);
    var.setBounds (take (seedW));
    lVar.setBounds (take (16));
    newVar.setBounds (take (btnW));
    r.removeFromLeft (6);
    back.setBounds (take (26));
    histLabel.setBounds (take (48));
    fwd.setBounds (take (26));
    favBtn.setBounds (take (54));
    r.removeFromLeft (4);
    juce::Component* right[] = { &exportBtn, &dragBtn, &previewBtn, &syncBtn, &midiOutBtn, &browserBtn };
    const int n = 6, gap = 5;
    const int each = (r.getWidth() - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i)
    {
        right[i]->setBounds (r.removeFromLeft (each));
        r.removeFromLeft (gap);
    }
}
} // namespace gfb::ui
