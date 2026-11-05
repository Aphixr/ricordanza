#pragma once

#include <string>
#include <unordered_map>

#include "../gl/shader.hpp"

class ShaderManager
{

public:

    static gl::Shader& get(const std::string& path);

private:

    static std::unordered_map<std::string, gl::Shader>& get_map();

};
