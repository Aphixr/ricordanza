#pragma once

#include <string>
#include <stdexcept>

class FileLoadFailureException : public std::runtime_error
{

public:

    explicit FileLoadFailureException(const std::string& filename)
        : std::runtime_error("failed to load file '" + filename + "'")
    {
    }

};
