#pragma once

#include "transcriber.hpp"

class LiveTranscriber : public Transcriber
{
public:
    void run(const std::vector<float>& raw) override;
};
