#include "shader_manager.hpp"

#include <fstream>

#include "file_load_failure_exception.hpp"
#include "../util/logger.hpp"

gl::Shader& ShaderManager::get(const std::string& path)
{
    Logger::info("loading shader '" + path + "'");

    if (get_map().contains(path))
    {
        return get_map().at(path);
    }

    std::ifstream file_stream { path, std::ios::binary };
    if (!file_stream)
    {
        throw FileLoadFailureException(path);
    }

    std::string contents;
    file_stream.seekg(0, std::ios::end);
    contents.resize(file_stream.tellg());
    file_stream.seekg(0, std::ios::beg);
    file_stream.read(&contents[0], contents.size());
    file_stream.close();

    gl::ShaderType type { };
    if (path.ends_with(".vert.glsl"))
    {
        type = gl::ShaderType::Vertex;
    }
    else if (path.ends_with(".frag.glsl"))
    {
        type = gl::ShaderType::Fragment;
    }
    else if (path.ends_with(".geom.glsl"))
    {
        type = gl::ShaderType::Geometry;
    }
    else
    {
        throw std::runtime_error { "could not determine shader type for '" + path + "'" };
    }

    get_map().emplace(
        std::piecewise_construct,
        std::forward_as_tuple(path),
        std::forward_as_tuple(type, contents)
    );

    if (!get_map().at(path).get_compile_success())
    {
        throw std::runtime_error { "failed to compile shader '" + path + "'" };
    }

    Logger::info("done: loaded shader '" + path + "'");

    return get_map().at(path);
}

// this is necessary to ensure the map is guaranteed initialized when accessed
std::unordered_map<std::string, gl::Shader>& ShaderManager::get_map()
{
    static std::unordered_map<std::string, gl::Shader> instance;
    return instance;
}
