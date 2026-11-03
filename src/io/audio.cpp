#include "audio.hpp"

Audio::Audio(std::string_view path)
    : m_path { path }
    , m_data { }
{
    // mono, 32-bit floats, 44100 Hz
    ma_decoder_config config { ma_decoder_config_init(ma_format_f32, 1, 44100) };
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

    Logger::info(
        "successfully loaded audio file '" + m_path + "' (" +
        std::to_string((int)get_seconds() / 60) + " min " +
        std::to_string((int)get_seconds() % 60) + " sec)"
    );

    ma_decoder_uninit(&decoder);
}

const std::vector<float>& Audio::get_data() const
{
    return m_data;
}

std::string Audio::get_path() const
{
    return m_path;
}

double Audio::get_seconds() const
{
    return m_data.size() / 44100.0;
}
