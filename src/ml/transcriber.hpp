#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <onnxruntime_cxx_api.h>

#include "../util/chord.hpp"

struct Note
{
    int key;                // 0-87
    float start_time;       // in seconds
    float end_time;         // in seconds
    float onset_prob;       // 0-1
    float max_frame_prob;   // 0-1
};

struct TranscriberOptions
{
    bool connect_broken_notes;
    bool filter_out_overtones;
    bool filter_out_blips;
};

class Transcriber
{

public:

    Transcriber();

    virtual ~Transcriber() = default;

    virtual const std::vector<std::vector<Note>>& get_notes() const;

    virtual const std::vector<Chord>& get_chords() const;

    unsigned get_note_count() const;

    float get_duration() const;

    void set_options(TranscriberOptions options);

    virtual void run(const std::vector<float>& raw);

protected:

    TranscriberOptions m_options;

    // [key 0-87][note]
    std::vector<std::vector<Note>> m_notes;

    // sorted by time
    std::vector<Chord> m_chords;

    // [key 0-87][frame]
    std::vector<std::vector<float>> m_frame_probs;

    // [key 0-87][frame]
    std::vector<std::vector<float>> m_onset_probs;

    void generate_probabilities(const std::vector<float>& raw);

    void generate_notes();

    // last in seconds
    void generate_notes(float last);

    void filter_out_overtones();

    // last in notes
    void filter_out_overtones(int last);

    // start and end are the timestamps to examine in seconds
    void filter_out_overtones(float start_time, float end_time, int target_key);

    void filter_out_blips();

    void filter_out_blips(int last);

    void connect_broken_notes();

    void connect_broken_notes(int last);

    void generate_chords();

    // last in seconds
    void generate_chords(float last);

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

    static std::unordered_map<ChordType, std::vector<bool>> s_chord_shapes;

    static std::unordered_map<std::string, int> s_interval_to_index;

    static void init_ort(const std::string& model_path);

    static void init_chord_shapes();

    static void define_chord_shape(ChordType type, std::vector<std::string> shape);

    static ChordProbability get_chord_probability(ChordType type, std::vector<float> weights);

};
