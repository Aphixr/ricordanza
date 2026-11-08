#pragma once

#include <string>

#include "pitch.hpp"

enum class ChordType
{
    other = -1,

    // intervals
    int_unison, int_min_2, int_maj_2, int_min_3, int_maj_3, int_perf_4,
    int_tritone, int_perf_5, int_min_6, int_maj_6, int_min_7, int_maj_7,

    // 3-note chords
    maj, min, aug, dim, sus_2, sus_4,

    // 6th chords
    maj_6, it_6, fr_6, ger_6,

    // 7th chords
    dom_7, maj_7, min_7, half_dim_7, dim_7, dom_7_sus_2, dom_7_sus_4,

    // 9th chords
    dom_9, maj_9, min_9, sus_9, dom_7_flat_9, six_nine,

    // 11th chords
    dom_11, maj_11, min_11, sus_11,

    // 13th chords
    dom_13, maj_13, min_13, sus_13,

    // misc
    quartal, quintal, scriabin_mystic
};

struct ChordProbability
{
    Letter root;
    ChordType type;
    float prob;             // 0.f - 1.f
};

struct Chord
{
    Letter root;
    ChordType type;
    float prob;             // 0.f - 1.f
    float start_time;       // in seconds
    float end_time;         // in seconds
};

constexpr std::string chord_type_to_string(ChordType type)
{
    switch (type)
    {
        case ChordType::other: return "other";
        case ChordType::int_unison: return "unison";
        case ChordType::int_min_2: return "min 2 (interval)";
        case ChordType::int_maj_2: return "maj 2 (interval)";
        case ChordType::int_min_3: return "min 3 (interval)";
        case ChordType::int_maj_3: return "maj 3 (interval)";
        case ChordType::int_perf_4: return "perfect 4 (interval)";
        case ChordType::int_tritone: return "tritone (interval)";
        case ChordType::int_perf_5: return "perfect 5 (interval)";
        case ChordType::int_min_6: return "min 6 (interval)";
        case ChordType::int_maj_6: return "maj 6 (interval)";
        case ChordType::int_min_7: return "min 7 (interval)";
        case ChordType::int_maj_7: return "maj 7 (interval)";
        case ChordType::maj: return "maj";
        case ChordType::min: return "min";
        case ChordType::aug: return "aug";
        case ChordType::dim: return "dim";
        case ChordType::sus_2: return "sus 2";
        case ChordType::sus_4: return "sus 4";
        case ChordType::maj_6: return "maj 6";
        case ChordType::it_6: return "it 6";
        case ChordType::fr_6: return "fr 6";
        case ChordType::ger_6: return "ger 6";
        case ChordType::dom_7: return "dom 7";
        case ChordType::maj_7: return "maj 7";
        case ChordType::min_7: return "min 7";
        case ChordType::half_dim_7: return "half dim 7";
        case ChordType::dim_7: return "dim 7";
        case ChordType::dom_7_sus_2: return "dom 7 sus 2";
        case ChordType::dom_7_sus_4: return "dom 7 sus 4";
        case ChordType::dom_9: return "dom 9";
        case ChordType::maj_9: return "maj 9";
        case ChordType::min_9: return "min 9";
        case ChordType::sus_9: return "sus 9";
        case ChordType::dom_7_flat_9: return "dom 7 flat 9";
        case ChordType::six_nine: return "six nine";
        case ChordType::dom_11: return "dom 11";
        case ChordType::maj_11: return "maj 11";
        case ChordType::min_11: return "min 11";
        case ChordType::sus_11: return "sus 11";
        case ChordType::dom_13: return "dom 13";
        case ChordType::maj_13: return "maj 13";
        case ChordType::min_13: return "min 13";
        case ChordType::sus_13: return "sus 13";
        case ChordType::quartal: return "quartal";
        case ChordType::quintal: return "quintal";
        case ChordType::scriabin_mystic: return "scriabin mystic";
    }
    return ""; // should be unreachable
}
