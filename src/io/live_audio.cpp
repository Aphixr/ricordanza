#include "live_audio.hpp"

#include <mutex>
#include <stdexcept>

LiveAudio::LiveAudio()
{
    if (!s_is_init)
    {
        init();
    }
}

LiveAudio::~LiveAudio()
{
    stop();
    s_buffer.clear();
    s_read_index = 0;
}

void LiveAudio::init()
{
    ma_device_config device_config = ma_device_config_init(ma_device_type_capture);
    device_config.capture.format = ma_format_f32;
    device_config.capture.channels = 1;
    device_config.sampleRate = 22050;
    device_config.dataCallback = audio_capture_callback;

    ma_result result = ma_device_init(NULL, &device_config, &s_device);
    if (result != MA_SUCCESS)
    {
        throw std::runtime_error("failed to initialize audio capture device");
    }

    s_is_init = true;
}

float LiveAudio::get_duration() const
{
    return (float)s_buffer.size() / 22050.f;
}

bool LiveAudio::is_recording() const
{
    return ma_device_is_started(&s_device);
}

void LiveAudio::start()
{
    ma_result result = ma_device_start(&s_device);
    if (result != MA_SUCCESS)
    {
        throw std::runtime_error("failed to start audio capture device");
    }
}

void LiveAudio::stop()
{
    ma_result result = ma_device_stop(&s_device);
    if (result != MA_SUCCESS)
    {
        throw std::runtime_error("failed to stop audio capture device");
    }
}

std::vector<float> LiveAudio::get_latest_unread_samples() const
{
    std::lock_guard<std::mutex> lock { buffer_mutex };

    if (s_buffer.size() - s_read_index < 43844)
    {
        return { };
    }

    std::vector<float> latest { };
    latest.resize(s_buffer.size() - s_read_index);
    std::copy(s_buffer.begin() + s_read_index, s_buffer.end(), latest.begin());

    s_read_index = s_buffer.size() - 1;

    return latest;
}

void LiveAudio::audio_capture_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    const float* incoming { (const float*)input };
    const size_t prev_end { s_buffer.size() - 1 + 1 };

    std::lock_guard<std::mutex> lock { buffer_mutex };

    s_buffer.insert(s_buffer.end(), incoming, incoming + frame_count);

    // normalize this latest chunk of stuff
    float max_amp { 0.f };
    for (size_t i { prev_end }; i < s_buffer.size(); i++)
    {
        if (std::abs(s_buffer[i]) > max_amp)
        {
            max_amp = std::abs(s_buffer[i]);
        }
    }
    if (max_amp > 0.f)
    {
        for (size_t i { prev_end }; i < s_buffer.size(); i++)
        {
            s_buffer[i] /= max_amp;
        }
    }

    // worry about this later
    // if (s_buffer.size() > s_max_buffer_size)
    // {
    //     size_t overflow { s_buffer.size() - s_max_buffer_size };
    //     s_buffer.erase(s_buffer.begin(), s_buffer.begin() + overflow);
    // }
}
