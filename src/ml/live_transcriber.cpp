#include "live_transcriber.hpp"

void LiveTranscriber::run(const std::vector<float>& raw)
{
    generate_probabilities(raw);

    generate_notes(get_duration() - m_chords.at(m_chords.size() - 1).end_time);

    if (m_options.filter_out_overtones)
    {
        filter_out_overtones(100);
    }

    if (m_options.connect_broken_notes)
    {
        connect_broken_notes(100);
    }

    if (m_options.filter_out_blips)
    {
        filter_out_blips(100);
    }

    generate_chords(get_duration() - m_chords.at(m_chords.size() - 1).end_time);
}
