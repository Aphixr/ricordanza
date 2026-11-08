#include "transcriber.hpp"

#include <cmath>

#include "../util/logger.hpp"
#include "../util/pitch.hpp"

using namespace std::literals;

Ort::Env Transcriber::s_env { ORT_LOGGING_LEVEL_WARNING, "ricordanza" };
Ort::Session Transcriber::s_session { nullptr };
Ort::MemoryInfo Transcriber::s_memory_info { Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault) };

std::vector<int64_t> Transcriber::s_input_shape { 1, Transcriber::s_chunk_size, 1 };

std::string Transcriber::s_input_name { };
std::string Transcriber::s_note_name { };
std::string Transcriber::s_onset_name { };
std::string Transcriber::s_contour_name { };

std::unordered_map<ChordType, std::vector<bool>> Transcriber::s_chord_shapes { };

std::unordered_map<std::string, int> Transcriber::s_interval_to_index { };

void Transcriber::init_ort(const std::string& model_path)
{
    Logger::info("initializing onnx model '" + model_path + "'...");

    // 2 threads
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(2);

    s_session = Ort::Session(s_env, model_path.c_str(), session_options);

    // create default allocator to handle the string memory
    Ort::AllocatorWithDefaultOptions allocator;

    // dynamically grab the 1 input and 3 output names baked into model
    // outputs should be 0 = Note, 1 = Onset, 2 = Contour
    // "note": pitch probabilities
    // "onset": whether key was just struck
    // "contour": micro-pitch bending (ignore)
    Ort::AllocatedStringPtr input_name_ptr = s_session.GetInputNameAllocated(0, allocator);
    Ort::AllocatedStringPtr out_note_ptr = s_session.GetOutputNameAllocated(0, allocator);
    Ort::AllocatedStringPtr out_onset_ptr = s_session.GetOutputNameAllocated(1, allocator);
    Ort::AllocatedStringPtr out_contour_ptr = s_session.GetOutputNameAllocated(2, allocator);

    s_input_name = input_name_ptr.get();
    s_note_name = out_note_ptr.get();
    s_onset_name = out_onset_ptr.get();
    s_contour_name = out_contour_ptr.get();

    s_is_ort_init = true;

    Logger::info("input names: "s + s_input_name);
    Logger::info("output names: "s + s_note_name + ", "s + s_onset_name + ", "s + s_contour_name);
    Logger::info("done: initialized onnx model '" + model_path + "'");
}

void Transcriber::init_chord_shapes()
{
    Logger::info("initializing chord shapes...");
    
    s_interval_to_index.insert({ "1", 0 });
    s_interval_to_index.insert({ "2-", 1 });
    s_interval_to_index.insert({ "2", 2 });
    s_interval_to_index.insert({ "2+", 3 });
    s_interval_to_index.insert({ "3-", 3 });
    s_interval_to_index.insert({ "3", 4 });
    s_interval_to_index.insert({ "3+", 5 });
    s_interval_to_index.insert({ "4-", 4 });
    s_interval_to_index.insert({ "4", 5 });
    s_interval_to_index.insert({ "4+", 6 });
    s_interval_to_index.insert({ "5-", 6 });
    s_interval_to_index.insert({ "5", 7 });
    s_interval_to_index.insert({ "5+", 8 });
    s_interval_to_index.insert({ "6-", 8 });
    s_interval_to_index.insert({ "6", 9 });
    s_interval_to_index.insert({ "6+", 10 });
    s_interval_to_index.insert({ "7-", 10 });
    s_interval_to_index.insert({ "7", 11 });
    s_interval_to_index.insert({ "8", 0 });
    s_interval_to_index.insert({ "9-", 1 });
    s_interval_to_index.insert({ "9", 2 });
    s_interval_to_index.insert({ "9+", 3 });
    s_interval_to_index.insert({ "10-", 3 });
    s_interval_to_index.insert({ "10", 4 });
    s_interval_to_index.insert({ "10+", 5 });
    s_interval_to_index.insert({ "11-", 4 });
    s_interval_to_index.insert({ "11", 5 });
    s_interval_to_index.insert({ "11+", 6 });
    s_interval_to_index.insert({ "13-", 8 });
    s_interval_to_index.insert({ "13", 9 });
    s_interval_to_index.insert({ "13+", 10 });
    s_interval_to_index.insert({ "16", 2 });
    s_interval_to_index.insert({ "17", 4 });
    s_interval_to_index.insert({ "21", 11 });

    define_chord_shape(ChordType::int_unison, { "1" });
    define_chord_shape(ChordType::int_min_2, { "1", "2-" });
    define_chord_shape(ChordType::int_maj_2, { "1", "2" });
    define_chord_shape(ChordType::int_min_3, { "1", "3-" });
    define_chord_shape(ChordType::int_maj_3, { "1", "3" });
    define_chord_shape(ChordType::int_perf_4, { "1", "4" });
    define_chord_shape(ChordType::int_tritone, { "1", "4+" });
    define_chord_shape(ChordType::int_perf_5, { "1", "5" });
    define_chord_shape(ChordType::int_min_6, { "1", "6-" });
    define_chord_shape(ChordType::int_maj_6, { "1", "6-" });
    define_chord_shape(ChordType::int_min_7, { "1", "7-" });
    define_chord_shape(ChordType::int_maj_7, { "1", "7" });
    define_chord_shape(ChordType::maj, { "1", "3", "5" });
    define_chord_shape(ChordType::min, { "1", "3-", "5" });
    define_chord_shape(ChordType::aug, { "1", "3", "5+" });
    define_chord_shape(ChordType::dim, { "1", "3-", "5-" });
    define_chord_shape(ChordType::sus_2, { "1", "2", "5" });
    define_chord_shape(ChordType::sus_4, { "1", "4", "5" });
    define_chord_shape(ChordType::maj_6, { "1", "3", "5", "6" });
    define_chord_shape(ChordType::dom_7, { "1", "3", "5", "7-" });
    define_chord_shape(ChordType::maj_7, { "1", "3", "5", "7" });
    define_chord_shape(ChordType::min_7, { "1", "3-", "5", "7-" });
    define_chord_shape(ChordType::half_dim_7, { "1", "3-", "5-", "7-" });
    define_chord_shape(ChordType::dim_7, { "1", "3-", "4+", "6" });
    define_chord_shape(ChordType::dom_7_sus_2, { "1", "2", "5", "7-" });
    define_chord_shape(ChordType::dom_7_sus_4, { "1", "4", "5", "7-" });
    define_chord_shape(ChordType::dom_9, { "1", "3", "5", "7-", "9" });
    define_chord_shape(ChordType::maj_9, { "1", "3", "5", "7", "9" });
    define_chord_shape(ChordType::min_9, { "1", "3-", "5", "7-", "9" });
    define_chord_shape(ChordType::sus_9, { "1", "4", "5", "7-", "9" });
    define_chord_shape(ChordType::dom_7_flat_9, { "1", "3", "5", "7-", "9-" });
    define_chord_shape(ChordType::six_nine, { "1", "3", "5", "6", "9" });
    define_chord_shape(ChordType::quartal, { "1", "4", "7-", "10-", "13-" });
    define_chord_shape(ChordType::quintal, { "1", "5", "9", "13", "17", "21" });
    define_chord_shape(ChordType::scriabin_mystic, { "1", "4+", "7-", "10", "13", "16" });

    Logger::info("done: initialized chord shapes");
}

void Transcriber::define_chord_shape(ChordType type, std::vector<std::string> shape)
{
    std::vector<bool> bool_shape;
    bool_shape.resize(12, false);

    for (std::string interval : shape)
    {
        bool_shape[s_interval_to_index[interval]] = true;
    }

    s_chord_shapes.emplace(type, bool_shape);
}

Transcriber::Transcriber()
    : m_options { false }
    , m_notes { }
    , m_frame_probs { }
    , m_onset_probs { }
{
    m_notes.resize(88);
    m_frame_probs.resize(88);
    m_onset_probs.resize(88);

    if (!s_is_ort_init)
    {
        init_ort("basic_pitch.onnx");
    }

    if (s_chord_shapes.size() == 0)
    {
        init_chord_shapes();
    }
}

const std::vector<std::vector<Note>>& Transcriber::get_notes() const
{
    return m_notes;
}

const std::vector<Chord>& Transcriber::get_chords() const
{
    return m_chords;
}

unsigned Transcriber::get_note_count() const
{
    unsigned count { 0 };
    for (const std::vector<Note>& key_notes : m_notes)
    {
        count += key_notes.size();
    }
    return count;
}

void Transcriber::set_options(TranscriberOptions options)
{
    m_options = options;
}

void Transcriber::run(const std::vector<float>& raw)
{
    generate_probabilities(raw);

    generate_notes();

    if (m_options.filter_out_overtones)
    {
        filter_out_overtones();
    }

    if (m_options.connect_broken_notes)
    {
        connect_broken_notes();
    }

    if (m_options.filter_out_blips)
    {
        filter_out_blips();
    }

    generate_chords();
}

void Transcriber::generate_probabilities(const std::vector<float>& raw)
{
    Logger::info("generating probabilities...");

    const char* input_names[] { s_input_name.c_str() };
    const char* output_names[] { s_note_name.c_str(), s_onset_name.c_str(), s_contour_name.c_str() };

    // loop through entire audio in chunks
    for (size_t offset { 0 }; offset + s_chunk_size <= raw.size(); offset += s_chunk_size)
    {
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            s_memory_info,
            const_cast<float*>(raw.data() + offset),
            s_chunk_size,
            s_input_shape.data(),
            s_input_shape.size()
        );
        
        // run neural network
        std::vector<Ort::Value> output_tensors = s_session.Run(
            Ort::RunOptions{nullptr},
            input_names, &input_tensor, 1,
            output_names, 3
        );

        // get note and onset probabilities
        float* frame_probs = output_tensors[0].GetTensorMutableData<float>();
        float* onset_probs = output_tensors[1].GetTensorMutableData<float>();

        // look at one key at a time
        for (int key { 0 }; key < 88; key++)
        {
            // loop through time
            for (int frame { 0 }; frame < s_frames_per_chunk; frame++)
            {
                int i { 88 * frame + key };
                m_frame_probs[key].push_back(frame_probs[i]);
                m_onset_probs[key].push_back(onset_probs[i]);
            }
        }
    }

    Logger::info("done: generated probabilities (" + std::to_string(m_frame_probs[0].size()) + " frames)");
}

void Transcriber::generate_notes()
{
    Logger::info("transcribing probabilities into vector of notes...");

    // generate notes
    // for each key
    for (int key { 0 }; key < 88; key++)
    {
        bool is_held { false };
        float start_time { 0.f }; // in seconds
        float held_onset_prob { 0.f };
        float start_frame_prob { 0.f };
        float max_frame_prob { 0.f };

        // for each frame in time
        for (int frame { 0 }; frame < m_frame_probs[key].size(); frame++)
        {
            float frame_prob { m_frame_probs[key][frame] };
            float onset_prob { m_onset_probs[key][frame] };

            if (!is_held)
            {
                if (onset_prob >= std::min(0.008f * key + 0.2f, 0.36f))
                {
                    is_held = true;
                    start_time = frame * s_frame_duration;
                    held_onset_prob = onset_prob;
                    start_frame_prob = frame_prob;
                }
            }
            else
            {
                if (frame_prob > max_frame_prob)
                {
                    max_frame_prob = frame_prob;
                }
                if (std::abs(frame_prob - start_frame_prob) > 0.04f)
                {
                    is_held = false;
                    float end_time = (frame) * s_frame_duration;

                    m_notes[key].push_back({
                        key,
                        start_time,
                        end_time,
                        held_onset_prob,
                        max_frame_prob
                    });
                }
            }
        }

        // if a note is still held when the audio ends, store it
        if (is_held)
        {
            m_notes[key].push_back({
                key,
                start_time,
                m_frame_probs[key].size() * s_frame_duration,
                held_onset_prob,
                max_frame_prob
            });
        }
    }

    Logger::info("done: transcribing probabilities into vector (" + std::to_string(get_note_count()) + " notes)");
}

void Transcriber::filter_out_overtones()
{
    Logger::info("filtering out harmonic overtones");

    // filter out harmonic overtones
    for (int key { 87 }; key >= 0; key--)
    {
        for (int i { 0 }; i < m_notes[key].size(); i++)
        {
            float start { m_notes[key][i].start_time };
            float end { m_notes[key][i].end_time };
            for (int j { i + 1 }; j < m_notes[key].size(); j++)
            {
                if (m_notes[key][j].start_time - end > 0.1f)
                {
                    break;
                }
                end = m_notes[key][j].end_time;
            }

            filter_out_overtones(start, end, key + 28); // 2 octaves plus major 3rd higher
            filter_out_overtones(start, end, key + 24); // 2 octaves higher
            filter_out_overtones(start, end, key + 19); // 1 octave plus perfect 5th higher
            filter_out_overtones(start, end, key + 12); // 1 octave higher
        }
    }

    Logger::info("done: filtered out harmonic overtones (" + std::to_string(get_note_count()) + " notes)");
}

void Transcriber::filter_out_overtones(float start_time, float end_time, int target_key)
{
    if (target_key >= 88)
    {
        return;
    }

    for (int i { 0 }; i < (int)(m_notes[target_key].size()); i++)
    {
        Note& curr { m_notes[target_key][i] };

        if (curr.start_time - start_time < -0.1f)
        {
            continue;
        }
        if (curr.end_time - end_time > 0.1f)
        {
            break;
        }

        std::vector<Note> connected_notes { curr };

        // find all connected notes
        for (int j { i + 1 }; j < (int)(m_notes[target_key].size()); j++)
        {
            Note& next { m_notes[target_key][j] };
            if (next.start_time - curr.end_time > 1.5f * s_frame_duration)
            {
                break;
            }
            connected_notes.push_back(next);
        }

        // get the weighted average of the onset and max frame probabilities of the connected notes
        float weighted_avg_onset_prob { 0.f };
        float weighted_avg_max_frame_prob { 0.f };
        float total_duration { 0.f };
        for (Note& note : connected_notes)
        {
            float duration { note.end_time - note.start_time };
            weighted_avg_onset_prob += note.onset_prob * duration;
            weighted_avg_max_frame_prob += note.max_frame_prob * duration;
            total_duration += duration;
        }
        if (total_duration > 0.f)
        {
            weighted_avg_onset_prob /= total_duration;
            weighted_avg_max_frame_prob /= total_duration;
        }

        // if the weighted average of the probabilities is less than the thresholds, remove all connected notes
        if (weighted_avg_onset_prob < 0.4f || weighted_avg_max_frame_prob < 0.5f)
        {
            for (int j { 0 }; j < (int)(connected_notes.size()); j++)
            {
                m_notes[target_key].erase(m_notes[target_key].begin() + i);
            }
            i--;
        }
    }
}

void Transcriber::filter_out_blips()
{
    Logger::info("filtering out blips...");

    for (int key { 0 }; key < 88; key++)
    {
        std::vector<Note> connected_notes { };

        for (int i { 0 }; i < (int)(m_notes[key].size()); i++)
        {
            Note& curr { m_notes[key][i] };

            std::vector<Note> connected_notes { curr };

            // find all connected notes
            // but break if it finds a note with high onset probability
            bool high_onset_found { false };
            for (int j { i + 1 }; j < (int)(m_notes[key].size()); j++)
            {
                Note& next { m_notes[key][j] };

                // end of connected notes
                if (next.start_time - curr.end_time > 1.5f * s_frame_duration)
                {
                    break;
                }

                // disregard connected notes if it finds a note with high onset probability
                if (next.onset_prob > 0.5f)
                {
                    high_onset_found = true;
                    break;
                }

                connected_notes.push_back(next);
            }

            if (high_onset_found)
            {
                continue;
            }

            // if the connected notes are too short, and there is enough silence before and after them, remove them
            float curr_start { connected_notes.front().start_time };
            float curr_end { connected_notes.back().end_time };
            if (curr_end - curr_start < 5.5f * s_frame_duration)
            {
                float prev_end { i == 0 ? 0.f : m_notes[key][i - 1].end_time };
                float next_start { curr_end == m_notes[key].back().end_time ? curr_end + 1.f : m_notes[key][i + connected_notes.size()].start_time };

                if (curr_start - prev_end > 0.5f && next_start - curr_end > 0.5f)
                {
                    // erase all connected notes
                    for (int j { 0 }; j < (int)(connected_notes.size()); j++)
                    {
                        m_notes[key].erase(m_notes[key].begin() + i);
                    }
                    i--;
                }
            }
        }
    }

    Logger::info("done: filtered out blips (" + std::to_string(get_note_count()) + " notes)");
}

void Transcriber::connect_broken_notes()
{
    Logger::info("connecting broken notes...");

    for (int key { 0 }; key < 88; key++)
    {
        for (int i { 0 }; i < (int)(m_notes[key].size()) - 1; i++)
        {
            Note& curr { m_notes[key][i] };
            Note& next { m_notes[key][i + 1] };
            if (next.start_time - curr.end_time < 0.2f)
            {
                curr.end_time = next.start_time;
            }
        }
    }

    Logger::info("done: connected broken notes");
}

// Looks at 0.1s chunks at a time, and each chunk are offset by 0.1s, regardless of chunk size.
// For each chunk, figure out how much each key is sustained.
// The longer it is sustained, the more weight it is given.
// Then it tries to use the 88 weights to match to a ChordType.
// Then it repeats all the aforementioned steps, but with larger and larger chunks, up to 10s chunks. (not yet implemented)
// The chunks grow by a factor of 2.
// I have no idea if this is going to work, be here's my reasoning:
// Starting with small chunks, it may detect a chord, or it may not (ChordType::unknown)
// It may be unable to detect the chord because the chord notes are too spread out.
// For example, imagine playing the notes C after 0 seconds, E after 2 seconds, G after 4 seconds.
// A small chunk of 0.1s would not be able to detect the a C major chord is played, and instead give a ChordType::int_...
// But at the same time, small chunks are needed to detect rapid changes in chords;
// e.g. imagine playing I, bII, V7, I in the span of 1 second.
// So, the larger chunks will attempt to fill up the gaps left by the smaller chunks, hopefully finding a real ChordType.
void Transcriber::generate_chords()
{
    const float chunk_size = 0.1f;
    const float chunk_offset = 0.1f;

    for (float s { chunk_offset - chunk_size }; s + chunk_size < m_frame_probs[0].size() * s_frame_duration; s += chunk_offset)
    {
        const float start { std::max(s, 0.f) };
        const float end { s + chunk_size };

        // 1 weight for each Letter
        // index 0 = A, index 11 = G-sharp
        std::vector<float> weights { };
        weights.resize(12, 0.f);

        // generate weights
        for (int key { 0 }; key < 88; key++)
        {
            for (Note& note : m_notes[key])
            {
                if (note.end_time <= start)
                {
                    continue;
                }
                if (note.start_time >= end)
                {
                    break;
                }

                weights[key % 12] += (note.end_time - note.start_time) / chunk_size;
                weights[key % 12] = std::min(weights[key % 12], 1.f);
            }
        }

        // get chord probabilities based on weights
        std::vector<ChordProbability> chord_probs { };
        chord_probs.reserve(47);

        for (const auto& [ type, shape ] : s_chord_shapes)
        {
            chord_probs.push_back(get_chord_probability(type, weights));
        }

        ChordProbability most_likely_chord { Letter::none, ChordType::other, 0.f };

        // find the chord with the greatest probability
        for (ChordProbability chord_prob : chord_probs)
        {
            if (chord_prob.prob > most_likely_chord.prob)
            {
                most_likely_chord = chord_prob;
            }
        }

        // insert this most likely chord into the vector
        m_chords.emplace_back(
            most_likely_chord.root,
            most_likely_chord.type,
            most_likely_chord.prob,
            start,
            end
        );
    }
}

ChordProbability Transcriber::get_chord_probability(ChordType type, std::vector<float> weights)
{
    int most_prob_root;
    float most_prob { 0.f };

    // first, check how many notes this chord expects
    int num_notes { 0 };
    for (bool yes : s_chord_shapes[type])
    {
        if (yes)
        {
            num_notes++;
        }
    }

    // we need to check all 12 different possible root notes
    // for each root
    for (int root { 0 }; root < 12; root++)
    {
        float prob { 0.f };

        // compare each weight to its corresponding position in the chord shape
        // no way this absolutely cryptic-looking algorithm worked on the first try
        // type of sht you see on a cs assignment
        int i { root };
        do
        {
            if (s_chord_shapes[type][(i - root + 12) % 12])
            {
                prob += weights[i];
            }
            else
            {
                prob -= weights[i];
            }
            i++;
            i %= 12;
        }
        while (i != root);

        prob /= num_notes;

        if (prob > most_prob)
        {
            most_prob = prob;
            most_prob_root = root;
        }
    }

    return { key_to_pitch(most_prob_root).letter, type, most_prob };
}
