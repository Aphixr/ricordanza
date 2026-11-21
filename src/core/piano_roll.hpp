#pragma once

#include <string>

#include <glm/glm.hpp>

#include "../io/audio.hpp"
#include "../io/live_audio.hpp"
#include "../gl/glacier.hpp"
#include "../ml/transcriber.hpp"
#include "../ml/live_transcriber.hpp"
#include "../util/chord.hpp"

class PianoRoll
{

public:

    PianoRoll();

    ~PianoRoll();

    void reset();

    void load_audio(std::string_view path);

    void transcribe_audio(TranscriberOptions options);

    void init_live_transcriber(TranscriberOptions options = { true, true, true });

    void start_live_transcriber();

    void stop_live_transcriber();

    unsigned get_note_index() const;

    unsigned get_note_count() const;

    float get_time() const;

    float get_duration() const;

    float get_pixels_per_second() const;

    std::string get_file_name() const;

    // current chord
    Chord get_chord() const;

    bool is_playing() const;

    // is live audio currently capturing input
    bool is_recording() const;

    void set_time(float time);

    void set_pixels_per_second(float pps);

    void set_max_onset_color(const glm::vec4& color);

    void set_min_onset_color(const glm::vec4& color);

    void set_bloom_intensity(float intensity);

    void play();

    void pause();

    void render();

    inline static const glm::vec4 default_min_onset_color { 0.0f, 0.4f, 1.2f, 1.f };
    
    inline static const glm::vec4 default_max_onset_color { 1.1f, 0.1f, -0.1f, 1.f };

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

    // live audio
    LiveAudio* m_live_audio;

    // live transcriber
    LiveTranscriber* m_live_transcriber;

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

    gl::Program m_composite_program;

    gl::Program m_blur_program;

    gl::VertexArray m_vao;

    gl::VertexArray m_notes_vao;

    gl::VertexArray m_quad_vao;

    gl::Buffer m_vbo;

    gl::Buffer m_notes_vbo;

    gl::Buffer m_inst_vbo;

    gl::Buffer m_quad_vbo;

    gl::Framebuffer m_note_fbo;

    gl::Framebuffer m_ping_pong_fbos[2];

    gl::Texture2D m_note_color_texture;

    gl::Texture2D m_ping_pong_textures[2];

    glm::mat4 m_projection;

    glm::vec4 m_max_onset_color { default_max_onset_color };

    glm::vec4 m_min_onset_color { default_min_onset_color };

    float m_bloom_intensity;

};
