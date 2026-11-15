#include "state/StateCodec.h"

#include "core/Serialization.h"

namespace gfb::state
{
namespace
{
const juce::Identifier kSchema ("schema"), kAlgo ("algorithmVersion"), kSettings ("settings"), kLocks ("locks"), kPattern ("pattern"),
    kHistory ("history"), kEntry ("entry"), kData ("data"), kCursor ("cursor"), kPreset ("preset"), kHostSync ("hostSync"),
    kMidiOut ("midiOut"), kPreviewBpm ("previewBpm"), kUseTaste ("useTaste"), kUi ("ui"), kW ("w"), kH ("h"), kDrawer ("drawer");

std::string str (const juce::var& v) { return v.toString().toStdString(); }
} // namespace

juce::ValueTree encode (const InstanceState& s)
{
    juce::ValueTree t (kRootId);
    t.setProperty (kSchema, kSchemaVersion, nullptr);
    t.setProperty (kAlgo, (int) kAlgorithmVersion, nullptr);
    t.setProperty (kSettings, juce::String (serial::mapToString (serial::settingsToMap (s.settings))), nullptr);
    t.setProperty (kLocks, juce::String (serial::mapToString (serial::locksToMap (s.locks))), nullptr);
    t.setProperty (kPattern, juce::String (serial::patternToString (s.pattern)), nullptr);
    t.setProperty (kPreset, s.presetIndex, nullptr);
    t.setProperty (kHostSync, s.hostSync, nullptr);
    t.setProperty (kMidiOut, s.midiOut, nullptr);
    t.setProperty (kPreviewBpm, s.previewBpm, nullptr);
    t.setProperty (kUseTaste, s.useTaste, nullptr);
    juce::ValueTree h (kHistory);
    h.setProperty (kCursor, s.historyCursor, nullptr);
    for (const auto& p : s.history)
    {
        juce::ValueTree e (kEntry);
        e.setProperty (kData, juce::String (serial::patternToString (p)), nullptr);
        h.appendChild (e, nullptr);
    }
    t.appendChild (h, nullptr);
    juce::ValueTree ui (kUi);
    ui.setProperty (kW, s.editorWidth, nullptr);
    ui.setProperty (kH, s.editorHeight, nullptr);
    ui.setProperty (kDrawer, s.drawerOpen, nullptr);
    t.appendChild (ui, nullptr);
    return t;
}

std::optional<InstanceState> decode (const juce::ValueTree& t)
{
    if (! t.isValid() || t.getType() != kRootId) return std::nullopt;
    const int schema = (int) t.getProperty (kSchema, 0);
    if (schema < 1 || schema > kSchemaVersion) return std::nullopt; // migration hook for future schemas
    InstanceState s;
    s.settings = serial::settingsFromMap (serial::mapFromString (str (t.getProperty (kSettings))));
    s.locks = serial::locksFromMap (serial::mapFromString (str (t.getProperty (kLocks))));
    if (auto p = serial::patternFromString (str (t.getProperty (kPattern)))) s.pattern = *p;
    s.presetIndex = juce::jlimit (0, 7, (int) t.getProperty (kPreset, 1));
    s.hostSync = (bool) t.getProperty (kHostSync, true);
    s.midiOut = (bool) t.getProperty (kMidiOut, false);
    s.useTaste = (bool) t.getProperty (kUseTaste, true);
    const double bpm = (double) t.getProperty (kPreviewBpm, 124.0);
    s.previewBpm = std::isfinite (bpm) ? juce::jlimit (40.0, 240.0, bpm) : 124.0;
    const auto h = t.getChildWithName (kHistory);
    if (h.isValid())
    {
        for (int i = 0; i < h.getNumChildren() && (int) s.history.size() < 64; ++i)
            if (auto p = serial::patternFromString (str (h.getChild (i).getProperty (kData)))) s.history.push_back (*p);
        s.historyCursor = s.history.empty() ? -1 : juce::jlimit (0, (int) s.history.size() - 1, (int) h.getProperty (kCursor, (int) s.history.size() - 1));
    }
    const auto ui = t.getChildWithName (kUi);
    if (ui.isValid())
    {
        s.editorWidth = juce::jlimit (1100, 2600, (int) ui.getProperty (kW, 1180));
        s.editorHeight = juce::jlimit (640, 1700, (int) ui.getProperty (kH, 760));
        s.drawerOpen = (bool) ui.getProperty (kDrawer, false);
    }
    return s;
}
} // namespace gfb::state
