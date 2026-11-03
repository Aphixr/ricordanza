#pragma once

#include <glad/glad.h>

#include <unordered_map>

namespace gl
{

class Buffer
{

public:

    Buffer(GLenum target);

    ~Buffer();

    Buffer(const Buffer&) = delete;

    Buffer& operator=(const Buffer&) = delete;

    GLuint get_id() const;

    void data(GLsizeiptr size, const GLvoid* data, GLenum usage);

    void bind();

    void unbind();

    void destroy();

    static void unbind(GLenum target);

    static Buffer* get_bound(GLenum target);

private:

    GLuint m_id;

    GLenum m_target;

    inline static std::unordered_map<GLenum, Buffer*> s_bound { };

};

} // namespace gl
