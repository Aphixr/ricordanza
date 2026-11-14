#pragma once

#include <glad/glad.h>

namespace gl
{

class Framebuffer
{

public:

    Framebuffer();

    ~Framebuffer();
    
    Framebuffer(const Framebuffer&) = delete;

    Framebuffer& operator=(const Framebuffer&) = delete;

    GLuint get_id() const;

    GLenum get_status() const;

    void attach_texture_2d(GLenum target, GLenum attachment, GLuint texture_id, GLint level);

    void blit(GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1, GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1, GLbitfield mask, GLenum filter);

    void bind(GLenum target=GL_FRAMEBUFFER);

    void unbind(GLenum target=GL_FRAMEBUFFER);

    void destroy();

private:

    GLuint m_id;

};

} // namespace gl
