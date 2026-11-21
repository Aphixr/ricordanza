#pragma once

#include <vector>
#include <mutex>

#include <miniaudio/miniaudio.h>

class LiveAudio
{

public:

    LiveAudio();

    ~LiveAudio();

    float get_duration() const;

    bool is_recording() const;

    void start();

    void stop();

    // returns a vector with at least 43,844 of the latest samples
    // if there are not enough samples, it returns an empty vector
    std::vector<float> get_latest_unread_samples() const;

    inline static std::mutex buffer_mutex { };

private:

    inline static ma_device s_device { };

    inline static std::vector<float> s_buffer { };

    // represents where the last read index is in the buffer
    inline static size_t s_read_index { };

    inline static int s_max_buffer_size { 22050 * 30 };

    inline static bool s_is_init { false };

    static void init();

    static void audio_capture_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);

};
