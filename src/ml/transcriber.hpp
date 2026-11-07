#pragma once

#include <vector>
#include <string>

#include <onnxruntime_cxx_api.h>

struct Note
{
    int key;                // 0-87
    float start_time;       // in seconds
    float end_time;         // in seconds
    float onset_prob;       // 0-1
    float max_frame_prob;   // 0-1
};

class Transcriber
{

public:

    Transcriber();

    const std::vector<std::vector<Note>>& get_notes() const;

    unsigned get_note_count() const;

    void run(const std::vector<float>& raw);

private:

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

    inline static bool s_is_ort_init { false };

    static Ort::Env s_env;
    
    static Ort::Session s_session;

    static Ort::MemoryInfo s_memory_info;

    static std::vector<int64_t> s_input_shape;

    static std::string s_input_name;

    static std::string s_note_name;

    static std::string s_onset_name;

    static std::string s_contour_name;

    static void init_ort(const std::string& model_path);

};
