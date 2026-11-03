#pragma once

#include <glad/glad.h>

#include <string>
#include <vector>

#include "shader.hpp"

namespace gl
{

class Program
{

public:

    Program();

    ~Program();

    Program(const Program&) = delete;

    Program& operator=(const Program&) = delete;

    GLuint get_id() const;

    bool get_link_success() const;

    void link();

    void use();

    void destroy();

    void attach(const Shader& shader);

    void attach(ShaderType type, std::string_view source);

    void detach(const Shader& shader);

    void set_uniform_1f(std::string_view name, GLfloat v0);

    void set_uniform_2f(std::string_view name, GLfloat v0, GLfloat v1);

    void set_uniform_3f(std::string_view name, GLfloat v0, GLfloat v1, GLfloat v2);

    void set_uniform_4f(std::string_view name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

    void set_uniform_1i(std::string_view name, GLint v0);

    void set_uniform_2i(std::string_view name, GLint v0, GLint v1);

    void set_uniform_3i(std::string_view name, GLint v0, GLint v1, GLint v2);

    void set_uniform_4i(std::string_view name, GLint v0, GLint v1, GLint v2, GLint v3);

private:

    GLuint m_id;

    // keeps track of internally created shaders for proper deletion later
    std::vector<GLuint> m_shader_ids;

    void destroy_internal_shaders();

};

} // namespace gl
