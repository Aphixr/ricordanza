#include "vertex_array.hpp"

#include <stdexcept>

#include "buffer.hpp"

namespace gl
{

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray()
{
    destroy();
}

GLuint VertexArray::get_id() const
{
    return m_id;
}

void VertexArray::attribute(GLuint index, GLuint size, GLenum type, GLboolean normalized, GLsizeiptr stride, GLvoid* offset)
{
    if (Buffer::get_bound(GL_ARRAY_BUFFER) == nullptr)
    {
        throw std::runtime_error("cannot set vertex attrib pointer when no buffer is bound to GL_ARRAY_BUFFER");
    }

    // tell GL how to read VBO (the buffer bound to GL_ARRAY_BUFFER)
    // f(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)
    glVertexAttribPointer(index, size, type, normalized, stride, offset);
}

void VertexArray::enable_attribute_array(GLuint index)
{
    glEnableVertexAttribArray(index);
}

void VertexArray::disable_attribute_array(GLuint index)
{
    glDisableVertexAttribArray(index);
}

void VertexArray::attribute_divisor(GLuint index, GLuint divisor)
{
    glVertexAttribDivisor(index, divisor);
}

void VertexArray::bind()
{
    glBindVertexArray(m_id);
}

void VertexArray::unbind()
{
    glBindVertexArray(0);
}

void VertexArray::destroy()
{
    glDeleteVertexArrays(1, &m_id);
}

} // namespace gl
