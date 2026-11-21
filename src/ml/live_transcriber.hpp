#pragma once

#include <thread>
#include <mutex>
#include <atomic>

#include "transcriber.hpp"
#include "../io/live_audio.hpp"

class LiveTranscriber : public Transcriber
{

public:

    LiveTranscriber(LiveAudio* live_audio);

    ~LiveTranscriber();

    bool is_running() const;

    void start();

    void stop();

    const std::vector<std::vector<Note>>& get_notes() const override;

    const std::vector<Chord>& get_chords() const override;

    void run(const std::vector<float>& raw) override;

    // locked when using or modifying notes
    inline static std::mutex mutex;

    // locked when using or copying m_notes_copy or m_chords_copy
    inline static std::mutex copy_mutex;

private:

    std::atomic<bool> m_is_running;

    std::atomic<bool> m_should_kill_thread;

    std::thread m_thread;

    size_t m_prev_notes_size;

    size_t m_prev_chords_size;

    std::vector<std::vector<Note>> m_notes_copy;

    std::vector<Chord> m_chords_copy;

    void run_thread(LiveAudio* live_audio);

};
