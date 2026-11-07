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

    void transcribe_audio(TranscriberOptions options);

    unsigned get_note_index() const;

    unsigned get_note_count() const;

    float get_time() const;

    float get_duration() const;

    float get_pixels_per_second() const;

    std::string get_file_name() const;

    bool is_playing() const;

    void set_time(float time);

    void set_pixels_per_second(float pps);

    void play();

    void pause();

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

    unsigned m_note_index;

    // represents the current playback time in seconds
    float m_time;

    float m_scroll_vel;

    float m_pixels_per_second;

    float m_pixels_per_second_vel;

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
