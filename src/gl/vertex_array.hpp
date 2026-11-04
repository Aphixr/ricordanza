#pragma once

#include <glad/glad.h>

namespace gl
{

class VertexArray
{

public:

    VertexArray();

    ~VertexArray();

    VertexArray(const VertexArray&) = delete;

    VertexArray& operator=(const VertexArray&) = delete;

    GLuint get_id() const;

    void attribute(GLuint index, GLuint size, GLenum type, GLboolean normalized, GLsizeiptr stride, GLvoid* offset);

    void enable_attribute_array(GLuint index);

    void disable_attribute_array(GLuint index);

    void attribute_divisor(GLuint index, GLuint divisor);

    void bind();

    void unbind();

    void destroy();

private:

    GLuint m_id;

};

} // namespace gl
