#pragma once

#include <string>
#include <vector>
#include <stdexcept>

#include <miniaudio/miniaudio.h>

#include "file_load_failure_exception.hpp"
#include "../util/logger.hpp"

class Audio
{

public:

    Audio(const std::string& path);

    // get the vector of 32-bit floats
    const std::vector<float>& get_data() const;

    // get path to audio file
    std::string get_path() const;

    // get duration of audio in seconds
    double get_seconds() const;

private:

    std::string m_path;

    std::vector<float> m_data;

};
