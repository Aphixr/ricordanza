#pragma once

#include <string>
#include <format>

// get current playback time as a string formatted as (-?)mm:ss.sss
inline std::string seconds_to_string(float seconds)
{
    return std::format(
        "{0}{1}:{2:0>6.3f}", // '0' = pad with 0, '>' = right align, '6' = width, ".3f" = 3 decimals
        seconds < 0 ? "-" : "",
        std::abs((int)seconds / 60),
        std::abs((float)((int)(seconds * 1000.f) % 60000) / 1000.f)
    );
}
