// Independent validation of our SMF writer with juce::MidiFile.
#include <juce_audio_basics/juce_audio_basics.h>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "midi/MidiFileExport.h"

using namespace gfb;

TEST_CASE (JuceMidi, JuceReaderAgreesWithCanonicalNotes)
{
    for (int bars : { 1, 2, 4, 8 })
        for (int fam = 0; fam < 2; ++fam)
        {
            GeneratorSettings s;
            s.bars = bars; s.family = (Family) fam; s.feel = Feel::Syncopated; s.styleSeed = (uint64_t) bars * 11 + (uint64_t) fam; s.swing = 60;
            const auto p = generatePattern (s);
            midi::ExportOptions o;
            o.tempoBpm = 123.0;
            const auto bytes = midi::writeSmf (p, o);
            juce::MemoryInputStream in (bytes.data(), bytes.size(), false);
            juce::MidiFile mf;
            int format = -1;
            REQUIRE (mf.readFrom (in, true, &format));
            CHECK_EQ (format, 0);
            CHECK_EQ (mf.getTimeFormat(), (short) kPPQ);
            REQUIRE (mf.getNumTracks() == 1);
            const auto* tr = mf.getTrack (0);
            juce::MidiMessageSequence seq (*tr);
            seq.updateMatchedPairs();
            std::vector<NoteEvent> got;
            double tempo = 0;
            int num = 0, den = 0;
            for (auto* e : seq)
            {
                const auto& m = e->message;
                if (m.isTempoMetaEvent()) tempo = 60.0 / m.getTempoSecondsPerQuarterNote();
                if (m.isTimeSignatureMetaEvent()) m.getTimeSignatureInfo (num, den);
                if (m.isNoteOn())
                {
                    REQUIRE (e->noteOffObject != nullptr); // no stuck notes
                    got.push_back ({ (int64_t) m.getTimeStamp(), (int64_t) (e->noteOffObject->message.getTimeStamp() - m.getTimeStamp()),
                                     (uint8_t) m.getNoteNumber(), m.getVelocity(), false, false });
                }
            }
            CHECK_NEAR (tempo, 123.0, 0.01);
            CHECK (num == 4 && den == 4);
            CHECK_NEAR (seq.getEndTime(), (double) p.lengthTicks(), 0.5);
            REQUIRE (got.size() == p.notes.size());
            for (size_t i = 0; i < got.size(); ++i)
            {
                CHECK_EQ (got[i].startTick, p.notes[i].startTick);
                CHECK_EQ (got[i].durationTicks, p.notes[i].durationTicks);
                CHECK_EQ ((int) got[i].midiNote, (int) p.notes[i].midiNote);
                CHECK_EQ ((int) got[i].velocity, (int) p.notes[i].velocity);
            }
        }
}
