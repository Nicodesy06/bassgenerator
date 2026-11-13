#include "midi/MidiDragSource.h"

#include "midi/MidiFileExport.h"

namespace gfb::mididrag
{
juce::File dragCacheDirectory()
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("GrooveForgeBass").getChildFile ("drag");
}

juce::Result exportTo (const Pattern& p, double bpm, const juce::File& target)
{
    if (p.notes.empty()) return juce::Result::fail ("The pattern is empty.");
    if (target.isDirectory()) return juce::Result::fail ("The target is a folder.");
    if (! target.getParentDirectory().createDirectory()) return juce::Result::fail ("Cannot create folder " + target.getParentDirectory().getFullPathName());
    midi::ExportOptions o;
    o.tempoBpm = bpm;
    const auto bytes = midi::writeSmf (p, o);
    const auto tmp = target.getSiblingFile (target.getFileName() + ".tmp");
    if (! tmp.replaceWithData (bytes.data(), bytes.size())) return juce::Result::fail ("Cannot write " + target.getFullPathName() + " (folder not writable?)");
    if (! tmp.replaceFileIn (target) && ! tmp.moveFileTo (target))
    {
        tmp.deleteFile();
        return juce::Result::fail ("Cannot replace " + target.getFullPathName() + " (file in use?)");
    }
    return juce::Result::ok();
}

juce::Result writeDragFile (const Pattern& p, double bpm, juce::File& outFile)
{
    const auto dir = dragCacheDirectory();
    if (! dir.createDirectory()) return juce::Result::fail ("Cannot create " + dir.getFullPathName());
    // unique per content + tempo so an older drag in progress is never overwritten
    const auto base = juce::String (midi::suggestedFileName (p)).upToLastOccurrenceOf (".mid", false, false);
    const auto tag = juce::String::toHexString ((juce::int64) (p.hash ^ (uint64_t) (bpm * 1000.0))).substring (0, 6);
    outFile = dir.getChildFile (base + "_" + tag + ".mid");
    if (outFile.existsAsFile() && outFile.getSize() > 0) return juce::Result::ok(); // identical content already there
    return exportTo (p, bpm, outFile);
}

int cleanupDragCache (double maxAgeHours)
{
    const auto dir = dragCacheDirectory();
    if (! dir.isDirectory()) return 0;
    int n = 0;
    const auto cutoff = juce::Time::getCurrentTime() - juce::RelativeTime::hours (maxAgeHours);
    for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "GFB_*.mid"))
        if (f.getLastModificationTime() < cutoff && f.deleteFile()) ++n;
    return n;
}
} // namespace gfb::mididrag
