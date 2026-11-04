#include "transcriber.hpp"

using namespace std::literals;

Transcriber::Transcriber(const std::string& model_path)
    : m_env { ORT_LOGGING_LEVEL_WARNING, "ricordanza" }
    , m_session { nullptr }
    , m_memory_info { Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault) }
{
    Logger::info("initializing onnx model '" + model_path + "'...");

    // limit to 1 thread
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);

    m_session = Ort::Session(m_env, model_path.c_str(), session_options);

    Logger::info("done: initialized onnx model '" + model_path + "'");
}

std::vector<Note> Transcriber::run(const std::vector<float>& raw)
{
    Logger::info("converting raw audio data into vector of notes...");

    // define the input shape
    std::vector<int64_t> input_shape = {1, s_chunk_size, 1};

    // create the default allocator to handle the string memory
    Ort::AllocatorWithDefaultOptions allocator;

    // dynamically grab the exact input string baked into model
    auto input_name_ptr = m_session.GetInputNameAllocated(0, allocator);
    const char* input_names[] = { input_name_ptr.get() };

    // dynamically grab the exact 3 output strings
    auto out_note_ptr = m_session.GetOutputNameAllocated(0, allocator);
    auto out_onset_ptr = m_session.GetOutputNameAllocated(1, allocator);
    auto out_contour_ptr = m_session.GetOutputNameAllocated(2, allocator);

    // to which output, but typically 0 = Note, 1 = Onset, 2 = Contour
    // "note": pitch probabilities
    // "onset": whether key was just struck
    // "contour": micro-pitch bending (ignore)
    const char* output_names[] = { out_note_ptr.get(), out_onset_ptr.get(), out_contour_ptr.get() };

    Logger::info("input names: "s + input_names[0]);
    Logger::info("output names: "s + output_names[0] + ", "s + output_names[1] + ", "s + output_names[2]);

    std::vector<Note> notes;

    // is the current key being held?
    bool is_held[88] { false };

    float start_time[88] { 0.f };
    float max_frame_prob[88] { 0.f };

    // loop through entire audio in chunks
    for (size_t offset { 0 }; offset + s_chunk_size <= raw.size(); offset += s_chunk_size)
    {
        // chunk start time in seconds
        float chunk_start_time { (float)offset / 22050.f };

        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            m_memory_info,
            const_cast<float*>(raw.data() + offset),
            s_chunk_size,
            input_shape.data(),
            input_shape.size()
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
                float frame_prob { frame_probs[i] };
                float onset_prob { onset_probs[i] };

                if (!is_held[key])
                {
                    if (onset_prob >= 0.5f || frame_prob >= 0.5f)
                    {
                        is_held[key] = true;
                        start_time[key] = chunk_start_time + (frame * s_frame_duration);
                        max_frame_prob[key] = frame_prob;
                    }
                }
                else
                {
                    if (frame_prob > max_frame_prob[key])
                    {
                        max_frame_prob[key] = frame_prob;
                    }
                    if (frame_prob < 0.15f)
                    {
                        is_held[key] = false;
                        float end_time = chunk_start_time + (frame * s_frame_duration);

                        // if the note was held for a very short amount of time
                        // ignore, probably was a little glitch
                        // otherwise, store it
                        if (end_time - start_time[key] > 0.06f)
                        {
                            notes.push_back({
                                key + 21,           // midi pitch
                                start_time[key],
                                end_time,
                                max_frame_prob[key] // amplitude
                            });
                        }
                    }
                }
            }
        }
    }

    for (int key { 0 }; key < 88; key++)
    {
        // if a note is still held when the audio ends, store it
        if (is_held[key])
        {
            notes.push_back({
                key + 21,
                start_time[key],
                raw.size() / 22050.f,
                max_frame_prob[key]
            });
        }
    }

    Logger::info("done: converting raw audio data into vector of notes");

    return notes;
}
