#pragma once

enum class Letter
{
    C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B
};

struct Pitch
{
    Letter letter; // 0-12
    int octave;
};

// converts midi pitch (0-127) to key index (0-87)
// returns -1 if outside of piano's range
constexpr int midi_to_index(int midi)
{
    return (midi <= 20 || midi >= 109) ? -1 : (midi - 21);
}

constexpr Pitch midi_to_pitch(int midi)
{
    Letter letter { };
    switch (midi % 12)
    {
        case 0:     letter = Letter::C;     break;
        case 1:     letter = Letter::Cs;    break;
        case 2:     letter = Letter::D;     break;
        case 3:     letter = Letter::Ds;    break;
        case 4:     letter = Letter::E;     break;
        case 5:     letter = Letter::F;     break;
        case 6:     letter = Letter::Fs;    break;
        case 7:     letter = Letter::G;     break;
        case 8:     letter = Letter::Gs;    break;
        case 9:     letter = Letter::A;     break;
        case 10:    letter = Letter::As;    break;
        case 11:    letter = Letter::B;     break;
    }
    return Pitch { letter, midi / 12 - 1 };
}
