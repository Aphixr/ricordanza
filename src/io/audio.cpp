#include "audio.hpp"

Audio::Audio(std::string_view path)
    : m_path { path }
    , m_data { }
    , m_engine { }
    , m_sound { }
    , m_playback_sample_rate { }
{
    Logger::info("loading audio file...");

    // mono, 32-bit floats, 22050 Hz
    ma_decoder_config config { ma_decoder_config_init(ma_format_f32, 1, 22050) };
    ma_decoder decoder { };

    if (ma_decoder_init_file(m_path.c_str(), &config, &decoder) != MA_SUCCESS)
    {
        throw FileLoadFailureException(m_path);
    }

    float chunk[4096];

    // read in 4096-frame chunks
    for (ma_uint64 frames_read { }; ; )
    {
        ma_decoder_read_pcm_frames(&decoder, chunk, 4096, &frames_read);
        if (frames_read == 0)
        {
            break;
        }
        m_data.insert(m_data.end(), chunk, chunk + frames_read);
    }

    ma_decoder_uninit(&decoder);

    ma_engine_init(NULL, &m_engine);
    ma_sound_init_from_file(&m_engine, m_path.c_str(), 0, NULL, NULL, &m_sound);
    m_playback_sample_rate = ma_engine_get_sample_rate(&m_engine);

    Logger::info(
        "done: loaded audio file '" + m_path + "' (" +
        std::to_string((int)get_duration() / 60) + " min " +
        std::to_string((int)get_duration() % 60) + " sec)"
    );
}

const std::vector<float>& Audio::get_data() const
{
    return m_data;
}

std::string Audio::get_path() const
{
    return m_path;
}

double Audio::get_duration() const
{
    return m_data.size() / 22050.0;
}

float Audio::get_time() const
{
    return (float)ma_sound_get_time_in_pcm_frames(&m_sound) / (float)m_playback_sample_rate;
}

bool Audio::is_playing() const
{
    return ma_sound_is_playing(&m_sound);
}

void Audio::play(float start_time)
{
    if (ma_sound_is_playing(&m_sound))
    {
        return;
    }
    ma_uint64 target_frame = (ma_uint64)(start_time * m_playback_sample_rate);
    ma_sound_seek_to_pcm_frame(&m_sound, target_frame);
    ma_sound_start(&m_sound);
}

void Audio::pause()
{
    if (!ma_sound_is_playing(&m_sound))
    {
        return;
    }
    ma_sound_stop(&m_sound);
}
