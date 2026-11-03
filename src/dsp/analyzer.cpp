#include "analyzer.hpp"

// STFT (short-time fourier transform) from what I understand
// the audio is going to be chopped up into a bunch of chunks called "windows"
// when STFT chops it up, it has a side effect of adding a high frequency click at the ends of the chunk
// to fix this, you have to multiply each chunk by a window function (e.g. Hann window)
// so that the ends of the chunks fades to zero
// then FFT happens
// also, the chunks overlap to replace the faded out to zero part

// now here's the crazy part
// we're gonna use Euler's formula: e^{i theta} = cos theta + i sin theta
// idk it can be used like this that's cool
// i'm not exactly sure what happens in the math behind the scenes
// i think it like compares the input waves against the sine/cosine or something
// but you end up needing to get the real part (cosine part) and imaginary part (sine part)
// and when you plot those parts as a point on the complex plane
// you find the distance between that point and origin to get the amplitude
// and you can determine other things

Analyzer::Analyzer(int window_size, int hop_size)
    : m_window_size { window_size }
    , m_hop_size { hop_size }
    , m_num_stft_bins { window_size / 2 + 1 }
    , m_in { }
    , m_out { }
    , m_plan { }
    , m_hann_window { }
    , m_filterbank { }
{
    if (m_hop_size > m_window_size)
    {
        throw std::invalid_argument("hop size should be less than window size");
    }
    if (m_window_size <= 0)
    {
        throw std::invalid_argument("window size must be a positive integer");
    }
    if (m_hop_size <= 0)
    {
        throw std::invalid_argument("hop size must be a positive integer");
    }

    // the following allocation functions allocates memory aligned in a special way
    // so the CPU can use SIMD (single instruction, multiple data) to do math super duper fast

    // allocate for input
    m_in = fftwf_alloc_real(m_window_size);

    // allocate for output
    // allocate half of window size plus one because
    // it has something to do with the FFT output being Hermitian symmetric
    // so we can forget about the second half since it's symmetrical
    // also Nyquist frequency
    m_out = fftwf_alloc_complex(m_num_stft_bins);

    // the plan to do hard math
    // dft = discrete fourier transform
    // r2c = real to complex
    // 1d = 1d array audio data input
    // FFTW_MEASURE allows FFTW to figure out the fastest algorithm based on CPU architecture and window size
    m_plan = fftwf_plan_dft_r2c_1d(m_window_size, m_in, m_out, FFTW_MEASURE);

    // calculate Hann window for later use
    m_hann_window.resize(m_window_size);
    for (int i { 0 }; i < m_window_size; i++)
    {
        m_hann_window[i] = 0.5f * (1.f - std::cos(2.f * M_PI * i / (window_size - 1)));
    }

    // calculate filterbank matrix for later use
    // the filters are triangularly shaped with peaks at the centers of each triangle
    // imagine a xy-plane
    // the x-axis represents the pitch frequency (Hz), the y-axis represents a weight between 0..1
    // the peak of each filter is a triangle sitting on this plane
    // its base touches the x-axis, and it has a peak near its center having a weight of 1

    // we first need create a vector to store the bin indices of where the peaks of each filter is located
    // the plus 2 is to include a index for the left end of the first (lowest frequency) filter
    // and an index for the right end of last (highest frequency) filter
    std::vector<int> peak_indices(num_filters + 2);
    for (int i { 0 }; i < num_filters + 2; i++)
    {
        float mel { min_mel + i * mel_step };
        float hz { mel_to_hz(mel) };

        // calculate the bin index for the peak from Hz
        peak_indices[i] = (int)std::floor(window_size * hz / 44100.f);
    }

    // now we can fill our 2D filterbank matrix with triangles
    m_filterbank.resize(num_filters, std::vector<float>(m_num_stft_bins, 0.f));
    for (int i { 0 }; i < num_filters; i++)
    {
        // bin indices of the left, right, and peak vertices of the triangle
        int lo_index { peak_indices[i] };
        int pk_index { peak_indices[i + 1] };
        int hi_index { peak_indices[i + 2] };

        // left (low) side of triangle goes up to 1
        for (int j { 0 }; j < pk_index; j++)
        {
            m_filterbank[i][j] = (float)(j - lo_index) / (pk_index - lo_index);
        }

        // right (high) side of triangle goes back down to 0
        for (int j { 0 }; j < pk_index; j++)
        {
            m_filterbank[i][j] = (float)(hi_index - j) / (hi_index - pk_index);
        }
    }
}

Analyzer::~Analyzer()
{
    fftwf_destroy_plan(m_plan);
    fftwf_free(m_in);
    fftwf_free(m_out);
}

std::vector<float> Analyzer::process(const std::vector<float>& data)
{
    // here, "frames" refer to the chunks/windows
    // a frame in DSP is different from a frame in IO
    const unsigned long num_frames { (data.size() - m_window_size) / m_hop_size };

    // the to-be-returned flattened Mel spectrogram
    std::vector<float> spectrogram;
    spectrogram.reserve(num_frames * num_filters);

    // for each frame (chunk)
    for (int frame { 0 }; frame < num_frames; frame++) {
        // the beginning of the chunk
        const int start_index { frame * m_hop_size };

        // multiply Hann window to current chunk and store the result to m_in
        for (int i { 0 }; i < m_window_size; i++)
        {
            m_in[i] = data[start_index + i] * m_hann_window[i];
        }

        // do math
        fftwf_execute(m_plan);

        // calculate (relative digital) power (energy) (frequency magnitude squared)
        // (frequency magnitude = sqrt(real * real + imag * imag))
        std::vector<float> power_spectrum(m_num_stft_bins);
        for (int i { 0 }; i < power_spectrum.size(); i++)
        {
            float real = m_out[i][0];
            float imag = m_out[i][1];

            // magnitude is the distance between the origin and the point on the complex plane
            // this is the amplitude (magnitude) squared
            power_spectrum[i] = real * real + imag * imag;
        }

        // multiply by Mel filterbank
        // this needs to be done immediately so that the floats in magnitudes are still in the CPU's cache
        // and not RAM so it can do math very quickly
        // std::vector<float> mel_frame(num_filters, 0.f);
        for (int i { 0 }; i < num_filters; i++)
        {
            float energy = 0.0f;

            // multiply the weights in the filterbank with its corresponding power and accumulate
            for (int j = 0; j < m_num_stft_bins; j++) {
                energy += m_filterbank[i][j] * power_spectrum[j]; 
            }

            // convert into (relative digital) decibels
            // add 1e-9 so that if energy is 0 (if it is dead silent), log(0) doesn't happen
            // add into flat vector
            spectrogram.push_back(10.0f * std::log10(energy + 1e-9f));
        }
    }

    return spectrogram;
}

