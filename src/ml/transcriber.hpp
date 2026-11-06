#pragma once

#include <vector>
#include <string>

#include <onnxruntime_cxx_api.h>

struct Note
{
    int midi_pitch;     // 0-127, or 21-108 (A0-C8)
    float start_time;   // in seconds
    float end_time;     // in seconds
    float amplitude;    // 0-1
};

class Transcriber
{

public:

    Transcriber(const std::string& model_path);

    const std::vector<std::vector<Note>>& get_notes() const;

    unsigned get_note_count() const;

    void run(const std::vector<float>& raw);

private:

    std::vector<int64_t> m_input_shape;

    std::string m_input_name;

    std::string m_note_name;

    std::string m_onset_name;

    std::string m_contour_name;

    Ort::Env m_env;
    
    Ort::Session m_session;

    Ort::MemoryInfo m_memory_info;

    // [key 0-87][note]
    std::vector<std::vector<Note>> m_notes;

    // [key 0-87][frame]
    std::vector<std::vector<float>> m_frame_probs;

    // [key 0-87][frame]
    std::vector<std::vector<float>> m_onset_probs;

    void generate_probabilities(const std::vector<float>& raw);

    void filter_out_overtones(Note fundamental, int target_key);

    // needed for basic pitch
    inline static const int s_chunk_size { 43844 };

    // needed for basic pitch
    inline static const int s_frames_per_chunk { 172 };

    // about 0.0115 seconds
    inline static const float s_frame_duration { (float)s_chunk_size / 22050.f / s_frames_per_chunk };

};
