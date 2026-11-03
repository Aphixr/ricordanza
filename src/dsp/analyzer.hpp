#pragma once

#include <vector>
#include <complex>
#include <fftw3.h>

#include "../util/mel_hz.hpp"

class Analyzer
{

public:

    // window_size: how big is the chunk
    // hop_size: the offset in number of frames (io) between each chunk
    Analyzer(int window_size, int hop_size);

    ~Analyzer();

    // returns a Mel spectrogram as a 2D vector [frame index][frequency index]
    // but it is flattened into 1D vector
    // so it's [dB for f1][dB for f2]...[dB for fn] ... [next fram chunk]
    // it's flattened so that the data stored in memory is contiguous
    std::vector<float> process(const std::vector<float>& data);

    inline static float min_hz { 20.f };

    inline static float max_hz { 44100.f / 2.f };

    inline static float min_mel { hz_to_mel(min_hz) };

    inline static float max_mel { hz_to_mel(max_hz) };

    inline static int num_filters { 256 };

    inline static float mel_step { (max_mel - min_mel) / (num_filters + 1) };

private:

    int m_window_size;

    int m_hop_size;

    int m_num_stft_bins;

    float* m_in;

    fftwf_complex* m_out;

    fftwf_plan m_plan;

    // used to fade the edges of chunks to zero
    std::vector<float> m_hann_window;

    // for generating the mel spectrogram
    // [filter index (0..127)][STFT bin index]
    std::vector<std::vector<float>> m_filterbank;

};
