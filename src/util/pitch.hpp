#pragma once

enum class Letter
{
    a, as, b, c, cs, d, ds, e, f, fs, g, gs, none
};

struct Pitch
{
    Letter letter; // 0-12
    int octave;
};

// converts midi pitch (0-127) to key index (0-87)
// returns -1 if outside of piano's range
constexpr int midi_to_key(int midi)
{
    return (midi <= 20 || midi >= 109) ? -1 : (midi - 21);
}

constexpr Pitch key_to_pitch(int key)
{
    Letter letter { };
    switch ((key + 21) % 12)
    {
        case 0:     letter = Letter::c;     break;
        case 1:     letter = Letter::cs;    break;
        case 2:     letter = Letter::d;     break;
        case 3:     letter = Letter::ds;    break;
        case 4:     letter = Letter::e;     break;
        case 5:     letter = Letter::f;     break;
        case 6:     letter = Letter::fs;    break;
        case 7:     letter = Letter::g;     break;
        case 8:     letter = Letter::gs;    break;
        case 9:     letter = Letter::a;     break;
        case 10:    letter = Letter::as;    break;
        case 11:    letter = Letter::b;     break;
    }
    return Pitch { letter, (key + 21) / 12 - 1 };
}

constexpr std::string key_to_letter_string(int key)
{
    switch ((key + 21) % 12)
    {
        case 0:     return "C";
        case 1:     return "D-flat";
        case 2:     return "D";
        case 3:     return "E-flat";
        case 4:     return "E";
        case 5:     return "F";
        case 6:     return "F-sharp";
        case 7:     return "G";
        case 8:     return "A-flat";
        case 9:     return "A";
        case 10:    return "B-flat";
        case 11:    return "B";
    }
    return ""; // this should be unreachable, but it satisfies compiler
}
