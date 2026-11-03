#include "program.hpp"

namespace gl
{

Program::Program()
    : m_id { glCreateProgram() }
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
}

void Program::use()
{
    glUseProgram(m_id);
}

void Program::destroy()
{
    glDeleteProgram(m_id);
}

void Program::attach(const Shader& shader)
{
    glAttachShader(m_id, shader.get_id());
}

void Program::detach(const Shader& shader)
{
    glDetachShader(m_id, shader.get_id());
}

void Program::set_uniform_1f(std::string_view identifier, GLfloat v0)
{
    glUniform1f(glGetUniformLocation(m_id, identifier.data()), v0);
}

void Program::set_uniform_2f(std::string_view identifier, GLfloat v0, GLfloat v1)
{
    glUniform2f(glGetUniformLocation(m_id, identifier.data()), v0, v1);
}

void Program::set_uniform_3f(std::string_view identifier, GLfloat v0, GLfloat v1, GLfloat v2)
{
    glUniform3f(glGetUniformLocation(m_id, identifier.data()), v0, v1, v2);
}

void Program::set_uniform_4f(std::string_view identifier, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    glUniform4f(glGetUniformLocation(m_id, identifier.data()), v0, v1, v2, v3);
}

void Program::set_uniform_1i(std::string_view identifier, GLint v0)
{
    glUniform1i(glGetUniformLocation(m_id, identifier.data()), v0);
}

void Program::set_uniform_2i(std::string_view identifier, GLint v0, GLint v1)
{
    glUniform2i(glGetUniformLocation(m_id, identifier.data()), v0, v1);
}

void Program::set_uniform_3i(std::string_view identifier, GLint v0, GLint v1, GLint v2)
{
    glUniform3i(glGetUniformLocation(m_id, identifier.data()), v0, v1, v2);
}

void Program::set_uniform_4i(std::string_view identifier, GLint v0, GLint v1, GLint v2, GLint v3)
{
    glUniform4i(glGetUniformLocation(m_id, identifier.data()), v0, v1, v2, v3);
}

} // namespace gl
