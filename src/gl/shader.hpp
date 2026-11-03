#pragma once

#include <glad/glad.h>

#include <string>

namespace gl
{

enum class ShaderType : GLenum
{
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER
};

class Shader
{

public:

    Shader() = delete;

    Shader(ShaderType type, std::string_view source);

    Shader(GLenum type, std::string_view source);

    ~Shader();

    Shader(const Shader&) = delete;

    Shader& operator=(const Shader&) = delete;

    GLuint get_id() const;

    ShaderType get_type() const;

    void destroy();

    bool get_compile_success() const;

private:

    GLuint m_id;

    ShaderType m_type;

};

} // namespace gl
