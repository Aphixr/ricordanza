#include "buffer.hpp"

namespace gl
{

Buffer::Buffer(GLenum target)
    : m_target { target }
{
    glGenBuffers(1, &m_id);
}

Buffer::~Buffer()
{
    destroy();
}

GLuint Buffer::get_id() const
{
    return m_id;
}

void Buffer::data(GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    glBufferData(m_target, size, data, usage);
}

void Buffer::bind()
{
    glBindBuffer(m_target, m_id);
    s_bound[m_target] = this;
}

void Buffer::unbind()
{
    glBindBuffer(m_target, 0);
    s_bound[m_target] = nullptr;
}

void Buffer::destroy()
{
    glDeleteBuffers(1, &m_id);
}

void Buffer::unbind(GLenum target)
{
    glBindBuffer(target, 0);
    s_bound[target] = nullptr;
}

Buffer* Buffer::get_bound(GLenum target)
{
    return s_bound[target];
}

} // namespace gl
