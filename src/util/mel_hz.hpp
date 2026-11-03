#pragma once

#include <cmath>

constexpr float hz_to_mel(float hz)
{
    return 2595.f * std::log10(1.f + hz / 700.f);
}

constexpr float mel_to_hz(float mel)
{
    return 700.f * (std::pow(10.f, mel / 2595.f) - 1.f);
}
