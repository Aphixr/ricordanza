#include "transcriber.hpp"

#include "../util/logger.hpp"

using namespace std::literals;

Ort::Env Transcriber::s_env { ORT_LOGGING_LEVEL_WARNING, "ricordanza" };
Ort::Session Transcriber::s_session { nullptr };
Ort::MemoryInfo Transcriber::s_memory_info { Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault) };

std::vector<int64_t> Transcriber::s_input_shape { 1, Transcriber::s_chunk_size, 1 };

std::string Transcriber::s_input_name { };
std::string Transcriber::s_note_name { };
std::string Transcriber::s_onset_name { };
std::string Transcriber::s_contour_name { };

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

Transcriber::Transcriber()
    : m_notes { }
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
}

const std::vector<std::vector<Note>>& Transcriber::get_notes() const
{
    return m_notes;
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

void Transcriber::run(const std::vector<float>& raw)
{
    generate_probabilities(raw);

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

    Logger::info("filtering out harmonic overtones");

    // filter out harmonic overtones
    for (int key { 0 }; key < 88; key++)
    {
        for (Note& note : m_notes[key])
        {
            filter_out_overtones(note, key + 12); // 1 octave higher
            filter_out_overtones(note, key + 19); // 1 octave plus perfect 5th higher
            filter_out_overtones(note, key + 24); // 2 octaves higher
            filter_out_overtones(note, key + 28); // 2 octaves plus major 3rd higher
        }
    }

    Logger::info("done: filtered out harmonic overtones (" + std::to_string(get_note_count()) + " notes)");
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

void Transcriber::filter_out_overtones(Note fundamental, int target_key)
{
    if (target_key >= 88)
    {
        return;
    }

    for (int i { 0 }; i < m_notes[target_key].size(); i++)
    {
        Note& target { m_notes[target_key][i] };
        if (target.end_time - fundamental.end_time > 0.1f)
        {
            break;
        }
        if (target.start_time - fundamental.start_time < -0.1f)
        {
            continue;
        }
        if (target.onset_prob < 0.42f * fundamental.onset_prob)
        {
            m_notes[target_key].erase(m_notes[target_key].begin() + i);
            i--;
        }
    }
}
