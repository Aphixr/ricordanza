#pragma once

#include <string>
#include <vector>

#include <miniaudio/miniaudio.h>

class Audio
{

public:

    Audio(std::string_view path);

    ~Audio();

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
    
    ma_uint32 m_playback_sample_rate;

    // for playback
    ma_sound m_sound;

    // for playback
    inline static ma_engine s_engine { };

    inline static bool s_is_ma_engine_init { false };

};
