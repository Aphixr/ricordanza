#include "shader.hpp"

namespace gl
{

Shader::Shader(ShaderType type, std::string_view source)
    : Shader { (GLenum)type, source }
{
}

Shader::Shader(GLenum type, std::string_view source)
    : m_type { type }, m_id { glCreateShader(type) }
{
    const char* cstr_source = source.data();
    glShaderSource(m_id, 1, &cstr_source, nullptr);
    glCompileShader(m_id);
}

Shader::~Shader()
{
    destroy();
}

void Shader::destroy()
{
    glDeleteShader(m_id);
}

bool Shader::get_compile_success() const
{
    GLint is_successful;
    glGetShaderiv(m_id, GL_COMPILE_STATUS, &is_successful);
    return static_cast<bool>(is_successful);
}

GLuint Shader::get_id() const
{
    return m_id;
}

ShaderType Shader::get_type() const
{
    return m_type;
}

} // namespace gl
