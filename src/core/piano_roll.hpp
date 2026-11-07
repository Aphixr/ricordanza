#pragma once

#include <string>

#include <glm/glm.hpp>

#include "../io/audio.hpp"
#include "../gl/glacier.hpp"
#include "../ml/transcriber.hpp"

class PianoRoll
{

public:

    PianoRoll();

    ~PianoRoll();

    void load_audio(std::string_view path);

    float get_time() const;

    void render();

    // returns x position of left edge of key
    static float get_key_x_pos(unsigned pitch);

    static bool is_white_key(unsigned pitch);

    static float get_key_width(unsigned key);

    static float get_white_key_width();

    static float get_black_key_width();

private:

    // loaded audio data
    Audio* m_audio;

    // transcribed data
    Transcriber* m_transcriber;

    // represents the current playback time in seconds
    float m_time;

    float m_scroll_vel;

    float m_pixels_per_second;

    // is it ready to render?
    bool m_is_ready;

    gl::Program m_piano_program;

    gl::Program m_notes_program;

    gl::VertexArray m_vao;

    gl::VertexArray m_notes_vao;

    gl::Buffer m_vbo;

    gl::Buffer m_notes_vbo;

    gl::Buffer m_inst_vbo;

    glm::mat4 m_projection;

};
