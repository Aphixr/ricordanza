#include "texture_2d.hpp"

namespace gl
{

Texture2D::Texture2D()
{
    glGenTextures(1, &m_id);
}

Texture2D::~Texture2D()
{
    destroy();
}

GLuint Texture2D::get_id() const
{
    return m_id;
}

void Texture2D::set_size(GLsizei width, GLsizei height)
{
    m_width = width;
    m_height = height;
}

void Texture2D::set_wrap(GLenum wrap_s, GLenum wrap_t)
{
    glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_t);
}

void Texture2D::set_filter(GLenum min_filter, GLenum mag_filter)
{
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
}

void Texture2D::data(GLint level, GLint internal_format, GLenum format, GLenum type, const GLvoid* data)
{
    glTexImage2D(target, level, internal_format, m_width, m_height, 0, format, type, data);
}

void Texture2D::gen_mipmap()
{
    glGenerateMipmap(target);
}

void Texture2D::activate(GLuint unit)
{
    glActiveTexture(unit);
}

void Texture2D::bind()
{
    glBindTexture(target, m_id);
}

void Texture2D::unbind()
{
    glBindTexture(target, 0);
}

void Texture2D::destroy()
{
    glDeleteTextures(1, &m_id);
}
    
} // namespace gl
