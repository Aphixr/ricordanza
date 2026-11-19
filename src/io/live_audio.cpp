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

void LiveAudio::audio_capture_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    const float* incoming { (const float*)input };

    std::lock_guard<std::mutex> lock { s_buffer_mutex };

    s_buffer.insert(s_buffer.end(), incoming, incoming + frame_count);

    if (s_buffer.size() > s_max_buffer_size)
    {
        size_t overflow { s_buffer.size() - s_max_buffer_size };
        s_buffer.erase(s_buffer.begin(), s_buffer.begin() + overflow);
    }
}
