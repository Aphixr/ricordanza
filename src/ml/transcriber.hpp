#pragma once

#include <cstddef>
#include <vector>
#include <string>

#include <onnxruntime_cxx_api.h>

#include "../dsp/analyzer.hpp"
#include "../util/logger.hpp"

struct Note
{
    int midi_pitch;     // 0-87 (A0-C8)
    float start_time;   // in seconds
    float end_time;     // in seconds
    float amplitude;    // 0-1
};

class Transcriber
{

public:

    Transcriber(const std::string& model_path);

    std::vector<Note> run(std::vector<float>& flat_mel, int num_frames, int hop_size);

private:

    std::vector<int64_t> m_input_shape;

    Ort::Env m_env;
    
    Ort::Session m_session;

    Ort::MemoryInfo m_memory_info;

};
