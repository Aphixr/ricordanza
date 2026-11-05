#pragma once

#include <string>
#include <vector>
#include <stdexcept>

#include <miniaudio/miniaudio.h>

#include "file_load_failure_exception.hpp"
#include "../util/logger.hpp"

class Audio
{

public:

    Audio(std::string_view path);

    // get the vector of 32-bit floats
    const std::vector<float>& get_data() const;

    // get path to audio file
    std::string get_path() const;

    // get duration of audio in seconds
    double get_duration() const;

    // get current playback time in seconds
    float get_time() const;

    // is the audio currently being played?
    bool is_playing() const;

    // play audio (start_time in seconds)
    void play(float start_time);

    // pause audio
    void pause();

private:

    std::string m_path;

    std::vector<float> m_data;

    // for playback
    ma_engine m_engine;

    // for playback
    ma_sound m_sound;
    
    ma_uint32 m_playback_sample_rate;

};
