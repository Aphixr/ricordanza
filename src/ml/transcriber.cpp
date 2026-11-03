#include "transcriber.hpp"

Transcriber::Transcriber(const std::string& model_path)
    : m_env { ORT_LOGGING_LEVEL_WARNING, "ricordanza" }
    , m_session { nullptr }
    , m_memory_info { Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault) }
{
    // limit to 1 thread
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);

    m_session = Ort::Session(m_env, model_path.c_str(), session_options);

    Logger::info("successfully loaded ONNX model '" + model_path + "'");
}

std::vector<Note> Transcriber::run(std::vector<float>& flat_mel, int num_frames, int hop_size)
{
    // define the shape: 1 audio file, number of time chunks/frames, number of filters/freq-bins, 1 channel
    std::vector<int64_t> input_shape = {1, num_frames, Analyzer::num_filters, 1};

    // wrap the vector into an ONNX tensor (no copying occurs)
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        m_memory_info,
        flat_mel.data(), 
        flat_mel.size(), 
        input_shape.data(), 
        input_shape.size()
    );

    // "note": pitch probabilities
    // "onset": whether key was just struck
    // "contour": micro-pitch bending (ignore)
    const char* input_names[] { "input" };
    const char* output_names[] { "note", "onset", "contour" };

    // run neural network
    std::vector<Ort::Value> output_tensors = m_session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 3);

    // get note and onset probabilities
    // shape for both: [1, num_frames, 88]
    float* frame_probs = output_tensors[0].GetTensorMutableData<float>();
    float* onset_probs = output_tensors[1].GetTensorMutableData<float>();

    std::vector<Note> notes;

    const float frame_duration { (float)hop_size / 44100.f };

    // look at one key at a time across the entire length of the audio
    for (int key { 0 }; key < 88; key++)
    {
        // is the current key being held?
        bool is_held { false };

        float start_time { 0.f };
        float max_frame_prob { 0.f };

        // loop through time
        for (int frame { 0 }; frame < num_frames; frame++)
        {
            int i { 88 * frame + key };
            float frame_prob { frame_probs[i] };
            float onset_prob { onset_probs[i] };

            if (!is_held)
            {
                if (frame_prob >= 0.5f || onset_prob >= 0.5f)
                {
                    is_held = true;
                    start_time = frame * frame_duration;
                    max_frame_prob = frame_prob;
                }
            }
            else
            {
                if (frame_prob > max_frame_prob)
                {
                    max_frame_prob = frame_prob;
                }
                if (frame_prob < 0.3f)
                {
                    is_held = false;
                    float end_time = frame * frame_duration;

                    // if the note was held for a very short amount of time
                    // ignore, probably was a little glitch
                    // otherwise, store it
                    if (end_time - start_time > 0.04f)
                    {
                        notes.push_back({
                            key + 21,       // midi pitch
                            start_time,
                            end_time,
                            max_frame_prob  // amplitude
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
                    frame * frame_duration,
                    max_frame_prob
                });
            }
        }
    }

    return notes;
}
