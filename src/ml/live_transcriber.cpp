#include "live_transcriber.hpp"

#include <functional>

LiveTranscriber::LiveTranscriber(LiveAudio* live_audio)
    : Transcriber { }
    , m_is_running { false }
    , m_should_kill_thread { false }
    , m_thread { std::bind(&LiveTranscriber::run_thread, this, live_audio) }
    , m_prev_notes_size { 0 }
    , m_prev_chords_size { 0 }
    , m_notes_copy { }
    , m_chords_copy { }
{
    m_notes_copy.resize(88);
}

LiveTranscriber::~LiveTranscriber()
{
    m_is_running = false;
    m_should_kill_thread = true;
    m_thread.join();
}

bool LiveTranscriber::is_running() const
{
    return m_is_running;
}

void LiveTranscriber::start()
{
    m_is_running = true;
}

void LiveTranscriber::stop()
{
    m_is_running = false;
}

const std::vector<std::vector<Note>>& LiveTranscriber::get_notes() const
{
    return m_notes_copy;
}

const std::vector<Chord>& LiveTranscriber::get_chords() const
{
    return m_chords_copy;
}

void LiveTranscriber::run(const std::vector<float>& raw)
{
    if (raw.size() < 43844)
    {
        return;
    }

    std::lock_guard<std::mutex> lock { mutex };

    generate_probabilities(raw);

    if (m_chords.size() > 0)
    {
        generate_notes(get_duration() - m_chords.at(m_chords.size() - 1).end_time);
    }
    else
    {
        generate_notes();
    }

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

    if (m_chords.size() > 0)
    {
        generate_chords(get_duration() - m_chords.at(m_chords.size() - 1).end_time);
    }
    else
    {
        generate_chords();
    }

    {
        std::lock_guard<std::mutex> lock { copy_mutex };
        m_notes_copy = m_notes;
        m_chords_copy = m_chords;
    }
}

void LiveTranscriber::run_thread(LiveAudio* live_audio)
{
    while (true)
    {
        if (m_should_kill_thread || live_audio == nullptr)
        {
            return;
        }

        if (!m_is_running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::vector<float> samples { live_audio->get_latest_unread_samples() };

        if (samples.size() == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        run(samples);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
