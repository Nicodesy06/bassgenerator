#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "state/FavoritesStore.h"
#include "state/StateCodec.h"
#include "ui/DesignSystem.h"

namespace gfb
{
class GrooveForgeProcessor;
}

namespace gfb::ui
{
juce::String describePattern (const Pattern& p);

/** Drawer with History (last 64 exact snapshots), Favorites (global, persistent) and Advanced groove options. */
class BrowserPanel : public juce::Component
{
public:
    explicit BrowserPanel (GrooveForgeProcessor& p);
    ~BrowserPanel() override;
    void refresh (const state::InstanceState& s);
    void refreshFavorites();
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void (const juce::String&, bool)> onMessage;

private:
    class HistoryModel;
    class FavoritesModel;
    GrooveForgeProcessor& proc;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Component historyPage, favPage, advPage;
    juce::ListBox historyList, favList;
    std::unique_ptr<HistoryModel> historyModel;
    std::unique_ptr<FavoritesModel> favModel;
    juce::TextButton favLoad { "LOAD" }, favRename { "RENAME" }, favDelete { "DELETE" }, favReveal { "SHOW FILE" };
    juce::Slider swing, previewBpm;
    juce::Label swingLabel, kickLabel, bpmLabel, info;
    juce::ComboBox kick;
    juce::TextButton resetBtn { "RESET TO DEFAULTS" };
    bool updating = false;
    state::InstanceState last;
    std::vector<Favorite> favs;
};
} // namespace gfb::ui
