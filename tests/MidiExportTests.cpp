#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "midi/MidiFileExport.h"

using namespace gfb;

TEST_CASE (Midi, RoundTripAllBarsAndFamilies)
{
    for (int bars : { 1, 2, 4, 8 })
        for (int fam = 0; fam < 2; ++fam)
            for (int feel = 0; feel < kNumFeels; ++feel)
            {
                GeneratorSettings s;
                s.bars = bars; s.family = (Family) fam; s.feel = (Feel) feel; s.styleSeed = (uint64_t) (bars * 10 + feel); s.varSeed = 3;
                s.swing = feel == 4 ? 60 : kAutoSwing;
                const auto p = generatePattern (s);
                midi::ExportOptions o;
                o.tempoBpm = 126.0;
                const auto bytes = midi::writeSmf (p, o);
                const auto r = midi::readSmf (bytes);
                REQUIRE (r.error.empty());
                CHECK_EQ (r.format, 0);
                CHECK_EQ (r.ppq, kPPQ);
                CHECK_EQ (r.numTracks, 1);
                CHECK_EQ (r.timeSigNum, 4);
                CHECK_EQ (r.timeSigDen, 4);
                CHECK_NEAR (r.tempoBpm, 126.0, 0.01);
                CHECK_EQ (r.endOfTrackTick, p.lengthTicks());
                REQUIRE (r.notes.size() == p.notes.size());
                for (size_t i = 0; i < p.notes.size(); ++i)
                {
                    CHECK_EQ (r.notes[i].startTick, p.notes[i].startTick);
                    CHECK_EQ (r.notes[i].durationTicks, p.notes[i].durationTicks);
                    CHECK_EQ ((int) r.notes[i].midiNote, (int) p.notes[i].midiNote);
                    CHECK_EQ ((int) r.notes[i].velocity, (int) p.notes[i].velocity);
                }
                CHECK (midi::writeSmf (p, o) == bytes); // deterministic bytes
            }
}

TEST_CASE (Midi, RepeatedSamePitchOrdersNoteOffFirst)
{
    Pattern p;
    p.bars = 1;
    for (int i = 0; i < 16; ++i) p.notes.push_back ({ (int64_t) i * 240, 240, 40, 100, false, false }); // fully legato same pitch
    REQUIRE (validatePattern (p).empty());
    const auto bytes = midi::writeSmf (p, {});
    const auto r = midi::readSmf (bytes);
    REQUIRE (r.error.empty());
    REQUIRE (r.notes.size() == 16);
    for (int i = 0; i < 16; ++i)
    {
        CHECK_EQ (r.notes[(size_t) i].startTick, (int64_t) i * 240);
        CHECK_EQ (r.notes[(size_t) i].durationTicks, 240);
    }
}

TEST_CASE (Midi, SlideOverlapExportsAsLegato)
{
    Pattern p;
    p.bars = 1;
    p.notes.push_back ({ 0, 270, 40, 110, true, true });
    p.notes.push_back ({ 240, 200, 43, 100, false, false });
    REQUIRE (validatePattern (p).empty());
    const auto r = midi::readSmf (midi::writeSmf (p, {}));
    REQUIRE (r.error.empty());
    REQUIRE (r.notes.size() == 2);
    CHECK (r.notes[0].endTick() > r.notes[1].startTick); // overlap preserved
}

TEST_CASE (Midi, ReaderRejectsCorruptFiles)
{
    GeneratorSettings s;
    auto bytes = midi::writeSmf (generatePattern (s), {});
    auto truncated = bytes;
    truncated.resize (truncated.size() / 2);
    CHECK (! midi::readSmf (truncated).error.empty());
    CHECK (! midi::readSmf ({ 'M', 'T' }).error.empty());
}

TEST_CASE (Midi, FileNameIsSafe)
{
    GeneratorSettings s;
    s.root = 1; s.styleSeed = 18446744073709551615ULL;
    const auto p = generatePattern (s);
    const auto n = midi::suggestedFileName (p);
    CHECK (n.size() > 10 && n.size() < 120);
    CHECK (n.substr (n.size() - 4) == ".mid");
    for (char c : n) CHECK ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-');
}
