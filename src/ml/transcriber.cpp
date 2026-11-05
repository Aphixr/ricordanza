#include "transcriber.hpp"

#include <cstring>

using namespace std::literals;

Transcriber::Transcriber(const std::string& model_path)
    : m_input_shape { 1, s_chunk_size, 1 }
    , m_env { ORT_LOGGING_LEVEL_WARNING, "ricordanza" }
    , m_session { nullptr }
    , m_memory_info { Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault) }
    , m_frame_probs { }
    , m_onset_probs { }
{
    Logger::info("initializing onnx model '" + model_path + "'...");

    // 2 threads
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(2);

    m_session = Ort::Session(m_env, model_path.c_str(), session_options);

    // create default allocator to handle the string memory
    Ort::AllocatorWithDefaultOptions allocator;

    // dynamically grab the 1 input and 3 output names baked into model
    // outputs should be 0 = Note, 1 = Onset, 2 = Contour
    // "note": pitch probabilities
    // "onset": whether key was just struck
    // "contour": micro-pitch bending (ignore)
    Ort::AllocatedStringPtr input_name_ptr = m_session.GetInputNameAllocated(0, allocator);
    Ort::AllocatedStringPtr out_note_ptr = m_session.GetOutputNameAllocated(0, allocator);
    Ort::AllocatedStringPtr out_onset_ptr = m_session.GetOutputNameAllocated(1, allocator);
    Ort::AllocatedStringPtr out_contour_ptr = m_session.GetOutputNameAllocated(2, allocator);

    m_input_name = input_name_ptr.get();
    m_note_name = out_note_ptr.get();
    m_onset_name = out_onset_ptr.get();
    m_contour_name = out_contour_ptr.get();

    m_frame_probs.resize(88);
    m_onset_probs.resize(88);

    Logger::info("input names: "s + m_input_name);
    Logger::info("output names: "s + m_note_name + ", "s + m_onset_name + ", "s + m_contour_name);
    Logger::info("done: initialized onnx model '" + model_path + "'");
}

void Transcriber::generate_probabilities(const std::vector<float>& raw)
{
    Logger::info("generating probabilities...");

    const char* input_names[] { m_input_name.c_str() };
    const char* output_names[] { m_note_name.c_str(), m_onset_name.c_str(), m_contour_name.c_str() };

    // loop through entire audio in chunks
    for (size_t offset { 0 }; offset + s_chunk_size <= raw.size(); offset += s_chunk_size)
    {
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            m_memory_info,
            const_cast<float*>(raw.data() + offset),
            s_chunk_size,
            m_input_shape.data(),
            m_input_shape.size()
        );
        
        // run neural network
        std::vector<Ort::Value> output_tensors = m_session.Run(
            Ort::RunOptions{nullptr},
            input_names, &inputTensor, 1,
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

std::vector<Note> Transcriber::run(const std::vector<float>& raw)
{
    Logger::info("converting raw audio data into vector of notes...");

    generate_probabilities(raw);

    std::vector<Note> notes;

    // for each key
    for (int key { 0 }; key < 88; key++)
    {
        bool is_held { false };
        float start_time { 0.f }; // in seconds
        float max_frame_prob { 0.f };

        // for each frame in time
        for (int frame { 0 }; frame < m_frame_probs[key].size(); frame++)
        {
            float frame_prob { m_frame_probs[key][frame] };
            float onset_prob { m_onset_probs[key][frame] };

            if (!is_held)
            {
                if (onset_prob >= 0.5f || frame_prob >= 0.5f)
                {
                    is_held = true;
                    start_time = frame * s_frame_duration;
                    max_frame_prob = frame_prob;
                }
            }
            else
            {
                if (frame_prob > max_frame_prob)
                {
                    max_frame_prob = frame_prob;
                }
                if (frame_prob < 0.15f)
                {
                    // look ahead a few frames to see if frame prob stays below threshold consistently
                    bool below_threshold_consistently { true };
                    for (int ahead { frame + 1 }; ahead <= frame + 7; ahead++)
                    {
                        if (m_frame_probs[key][ahead] >= 0.15f)
                        {
                            below_threshold_consistently = false;
                            break;
                        }
                    }
                    if (!below_threshold_consistently)
                    {
                        continue;
                    }

                    is_held = false;
                    float end_time = frame * s_frame_duration;

                    // if the note was held for a very short amount of time
                    // ignore, probably was a little glitch
                    // otherwise, store it
                    if (end_time - start_time < 0.06f)
                    {
                        continue; // continue loop frame
                    }
                    notes.push_back({
                        key + 21,           // midi pitch
                        start_time,
                        end_time,
                        max_frame_prob      // amplitude
                    });
                }
            }
        }

        // if a note is still held when the audio ends, store it
        if (is_held)
        {
            notes.push_back({
                key + 21,
                start_time,
                raw.size() / 22050.f,
                max_frame_prob
            });
        }
    }

    Logger::info("done: converting raw audio data into vector of notes");

    return notes;
}
