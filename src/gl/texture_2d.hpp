#pragma once

#include <glad/glad.h>

namespace gl
{

class Texture2D
{

public:

    Texture2D();

    ~Texture2D();
    
    Texture2D(const Texture2D&) = delete;

    Texture2D& operator=(const Texture2D&) = delete;

    GLuint get_id() const;

    void set_size(GLsizei width, GLsizei height);

    void set_wrap(GLenum wrap_s, GLenum wrap_t);

    void set_filter(GLenum min_filter, GLenum mag_filter);

    void data(GLint level, GLint internal_format, GLenum format, GLenum type, const GLvoid* data);

    void gen_mipmap();

    void activate(GLuint unit);

    void bind();

    void unbind();

    void destroy();

    inline static const GLuint target { GL_TEXTURE_2D };

private:

    GLuint m_id;

    GLsizei m_width;

    GLsizei m_height;

};

} // namespace gl
