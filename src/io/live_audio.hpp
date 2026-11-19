#pragma once

#include <vector>
#include <mutex>

#include <miniaudio/miniaudio.h>

class LiveAudio
{

public:

    LiveAudio();

    ~LiveAudio() = default;

    bool is_recording() const;

    void start();

    void stop();

    inline static std::vector<float> s_buffer { };

private:

    inline static ma_device s_device { };

    inline static std::mutex s_buffer_mutex { };

    inline static int s_max_buffer_size { 22050 * 3 };

    inline static bool s_is_init { false };

    static void init();
    
    static void audio_capture_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);

};
