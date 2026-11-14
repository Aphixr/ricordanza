#include "framebuffer.hpp"

namespace gl
{

Framebuffer::Framebuffer()
{
    glGenFramebuffers(1, &m_id);
}

Framebuffer::~Framebuffer()
{
    destroy();
}

GLuint Framebuffer::get_id() const
{
    return m_id;
}

GLenum Framebuffer::get_status() const
{
    return glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

void Framebuffer::attach_texture_2d(GLenum target, GLenum attachment, GLuint texture_id, GLint level)
{
    glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture_id, level);
}

void Framebuffer::blit(GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1, GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1, GLbitfield mask, GLenum filter)
{
    glBlitFramebuffer(src_x0, src_y0, src_x1, src_y1, dst_x0, dst_y0, dst_x1, dst_y1, mask, filter);
}

void Framebuffer::bind(GLenum target=GL_FRAMEBUFFER)
{
    glBindFramebuffer(target, m_id);
}

void Framebuffer::unbind(GLenum target=GL_FRAMEBUFFER)
{
    glBindFramebuffer(target, 0);
}

void Framebuffer::destroy()
{
    glDeleteFramebuffers(1, &m_id);
}

} // namespace gl
