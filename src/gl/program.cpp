#include "program.hpp"

#include <stdexcept>

namespace gl
{

Program::Program()
    : m_id { glCreateProgram() }
    , m_shader_ids { }
{
}

Program::~Program()
{
    destroy();
}

GLuint Program::get_id() const
{
    return m_id;
}

bool Program::get_link_success() const
{
    GLint is_successful;
    glGetProgramiv(m_id, GL_LINK_STATUS, &is_successful);
    return static_cast<bool>(is_successful);
}

void Program::link()
{
    glLinkProgram(m_id);
    destroy_internal_shaders();
}

void Program::use()
{
    glUseProgram(m_id);
}

void Program::destroy()
{
    destroy_internal_shaders();
    glDeleteProgram(m_id);
}

void Program::attach(const Shader& shader)
{
    glAttachShader(m_id, shader.get_id());
}

void Program::attach(ShaderType type, std::string_view source)
{
    const char* cstr { source.data() };
    GLuint shader { glCreateShader((GLenum)type) };
    glShaderSource(shader, 1, &cstr, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info);
        throw std::runtime_error(info);
    }

    glAttachShader(m_id, shader);
    m_shader_ids.push_back(m_id);
}

void Program::detach(const Shader& shader)
{
    glDetachShader(m_id, shader.get_id());
}

void Program::set_uniform_1f(std::string_view name, GLfloat v0)
{
    glUniform1f(glGetUniformLocation(m_id, name.data()), v0);
}

void Program::set_uniform_2f(std::string_view name, GLfloat v0, GLfloat v1)
{
    glUniform2f(glGetUniformLocation(m_id, name.data()), v0, v1);
}

void Program::set_uniform_3f(std::string_view name, GLfloat v0, GLfloat v1, GLfloat v2)
{
    glUniform3f(glGetUniformLocation(m_id, name.data()), v0, v1, v2);
}

void Program::set_uniform_4f(std::string_view name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    glUniform4f(glGetUniformLocation(m_id, name.data()), v0, v1, v2, v3);
}

void Program::set_uniform_1i(std::string_view name, GLint v0)
{
    glUniform1i(glGetUniformLocation(m_id, name.data()), v0);
}

void Program::set_uniform_2i(std::string_view name, GLint v0, GLint v1)
{
    glUniform2i(glGetUniformLocation(m_id, name.data()), v0, v1);
}

void Program::set_uniform_3i(std::string_view name, GLint v0, GLint v1, GLint v2)
{
    glUniform3i(glGetUniformLocation(m_id, name.data()), v0, v1, v2);
}

void Program::set_uniform_4i(std::string_view name, GLint v0, GLint v1, GLint v2, GLint v3)
{
    glUniform4i(glGetUniformLocation(m_id, name.data()), v0, v1, v2, v3);
}

void Program::set_uniform_mat4(std::string_view name, GLfloat* value)
{
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.data()), 1, GL_FALSE, value);
}

void Program::destroy_internal_shaders()
{
    for (GLuint shader : m_shader_ids)
    {
        glDetachShader(m_id, shader);
        glDeleteShader(shader);
    }
    m_shader_ids.clear();
}

} // namespace gl
