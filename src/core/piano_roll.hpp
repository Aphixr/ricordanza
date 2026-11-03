#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../io/audio.hpp"
#include "../gl/glacier.hpp"

class PianoRoll
{

public:

    PianoRoll();

    ~PianoRoll();

    void load_audio(std::string_view path);

    float get_time() const;

    void render();

    // returns x position of left edge of key
    static float midi_pitch_to_horizontal_position(unsigned pitch);

    static bool is_white_key(unsigned pitch);

    static float get_white_key_width();

    static float get_black_key_width();

private:

    // loaded audio data
    Audio* m_audio;

    // represents the current playback time in seconds
    float m_time;

    gl::Program m_shader_program;

    gl::VertexArray m_vao;

    gl::Buffer m_vbo;

    glm::mat4 m_projection;

};
