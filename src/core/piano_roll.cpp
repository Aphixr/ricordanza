#include "piano_roll.hpp"

#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "app.hpp"
#include "../io/shader_manager.hpp"
#include "../util/pitch.hpp"
#include "../util/time.hpp"
#include "../util/logger.hpp"
#include "../ml/transcriber.hpp"

using namespace std::literals;

float vertices[]
{
    0.f, 1.f,
    1.f, 0.f,
    0.f, 0.f,
    0.f, 1.f,
    1.f, 1.f,
    1.f, 0.f
};

float quad_vertices[]
{
    // positions   // tex coords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};

struct InstanceData
{
    glm::mat4 model;
    glm::vec4 color;
};

PianoRoll::PianoRoll()
    : m_audio { nullptr }
    , m_transcriber { nullptr }
    , m_live_audio { nullptr }
    , m_live_transcriber { nullptr }
    , m_note_index { 0 }
    , m_time { 0.f }
    , m_scroll_vel { 0.f }
    , m_pixels_per_second { 100.f }
    , m_pixels_per_second_vel { 0.f }
    , m_is_ready { false }
    , m_piano_program { }
    , m_notes_program { }
    , m_composite_program { }
    , m_blur_program { }
    , m_vao { }
    , m_notes_vao { }
    , m_quad_vao { }
    , m_vbo { GL_ARRAY_BUFFER }
    , m_notes_vbo { GL_ARRAY_BUFFER }
    , m_inst_vbo { GL_ARRAY_BUFFER }
    , m_quad_vbo { GL_ARRAY_BUFFER }
    , m_note_fbo { }
    , m_ping_pong_fbos { }
    , m_note_color_texture { }
    , m_ping_pong_textures { }
    , m_projection { glm::ortho(0.f, (float)App::get_window_width(), 0.f, (float)App::get_window_height(), -1.f, 1.f) }
    , m_bloom_intensity { 1.5f }
{
    m_piano_program.attach(ShaderManager::get("default.vert.glsl"));
    m_piano_program.attach(ShaderManager::get("solid.frag.glsl"));
    m_piano_program.link();

    m_notes_program.attach(ShaderManager::get("instance.vert.glsl"));
    m_notes_program.attach(ShaderManager::get("instance.frag.glsl"));
    m_notes_program.link();

    m_composite_program.attach(ShaderManager::get("post.vert.glsl"));
    m_composite_program.attach(ShaderManager::get("composite.frag.glsl"));
    m_composite_program.link();

    m_blur_program.attach(ShaderManager::get("post.vert.glsl"));
    m_blur_program.attach(ShaderManager::get("blur.frag.glsl"));
    m_blur_program.link();

    m_vao.bind();
    m_vbo.bind();
    m_vbo.data(sizeof(vertices), vertices, GL_STATIC_DRAW);
    m_vao.attribute(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    m_vao.enable_attribute_array(0);
    m_vao.unbind();
    m_vbo.unbind();

    m_notes_vao.bind();
    m_notes_vbo.bind();
    m_notes_vbo.data(sizeof(vertices), vertices, GL_STATIC_DRAW);
    m_notes_vao.attribute(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    m_notes_vao.enable_attribute_array(0);
    m_inst_vbo.bind();
    for (int loc { 1 }; loc <= 4; loc++)
    {
        m_notes_vao.enable_attribute_array(loc);
        m_notes_vao.attribute(loc, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)((loc - 1) * sizeof(glm::vec4)));
        m_notes_vao.attribute_divisor(loc, 1);
    }
    m_notes_vao.enable_attribute_array(5);
    m_notes_vao.attribute(5, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(glm::mat4)));
    m_notes_vao.attribute_divisor(5, 1);
    m_notes_vao.unbind();
    m_inst_vbo.unbind();

    m_quad_vao.bind();
    m_quad_vbo.bind();
    m_quad_vbo.data(sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    m_quad_vao.attribute(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    m_quad_vao.enable_attribute_array(0);
    m_quad_vao.attribute(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    m_quad_vao.enable_attribute_array(1);
    m_quad_vao.unbind();
    m_quad_vbo.unbind();

    m_note_fbo.bind();
    m_note_color_texture.bind();
    m_note_color_texture.set_size(App::get_window_width(), App::get_window_height());
    m_note_color_texture.data(0, GL_RGBA16F, GL_RGBA, GL_FLOAT, NULL);
    m_note_color_texture.set_wrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    m_note_color_texture.set_filter(GL_LINEAR, GL_LINEAR);
    m_note_fbo.attach_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_note_color_texture.get_id(), 0);

    for (int i { 0 }; i < 2; i++)
    {
        m_ping_pong_fbos[i].bind();
        m_ping_pong_textures[i].bind();
        m_ping_pong_textures[i].set_size(App::get_window_width() / 2, App::get_window_height() / 2);
        m_ping_pong_textures[i].data(0, GL_RGBA16F, GL_RGBA, GL_FLOAT, NULL);
        m_ping_pong_textures[i].set_wrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_ping_pong_textures[i].set_filter(GL_LINEAR, GL_LINEAR);
        m_ping_pong_fbos[i].attach_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_ping_pong_textures[i].get_id(), 0);
    }

    m_ping_pong_fbos[1].unbind();
    m_ping_pong_textures[1].unbind();

    m_composite_program.use();
    m_composite_program.set_uniform_1i("crisp_notes", 0);
    m_composite_program.set_uniform_1i("bloom_blur", 1);

    App::on_key_down(GLFW_KEY_SPACE, [this]()
    {
        if (m_audio)
        {
            if (!m_audio->is_playing())
            {
                m_time = std::max(m_time, 0.f);
                m_audio->play(m_time);
            }
            else
            {
                m_audio->pause();
            }
        }
        else if (m_live_audio)
        {
            if (!m_live_audio->is_recording())
            {
                m_live_audio->start();
            }
            else
            {
                m_live_audio->stop();
            }
        }
    });

    App::on_scroll([this](double x_offset, double y_offset)
    {
        if (!m_is_ready)
        {
            return;
        }
        if (App::is_key_pressed(GLFW_KEY_LEFT_CONTROL) || App::is_key_pressed(GLFW_KEY_RIGHT_CONTROL))
        {
            m_pixels_per_second_vel = -2.5f * y_offset;
        }
        else
        {
            m_scroll_vel = 1.2f * y_offset * (100.f / m_pixels_per_second);
        }
    });

    App::on_window_resize([this](int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        m_note_color_texture.bind();
        m_note_color_texture.set_size(width, height);
        m_note_color_texture.data(0, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);

        m_ping_pong_textures[0].bind();
        m_ping_pong_textures[0].set_size(width / 2, height / 2);
        m_ping_pong_textures[0].data(0, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);

        m_ping_pong_textures[1].bind();
        m_ping_pong_textures[1].set_size(width / 2, height / 2);
        m_ping_pong_textures[1].data(0, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);
        m_ping_pong_textures[1].unbind();
    });

    App::on_render([this]()
    {
        if (!m_is_ready)
        {
            return;
        }
        render();
    });
}

PianoRoll::~PianoRoll()
{
    reset();
}

void PianoRoll::reset()
{
    Logger::info("resetting...");

    m_is_ready = false;

    if (m_audio != nullptr)
    {
        m_audio->pause();
        delete m_audio;
        m_audio = nullptr;
    }
    if (m_transcriber != nullptr)
    {
        delete m_transcriber;
        m_transcriber = nullptr;
    }

    if (m_live_audio && m_live_audio->is_recording())
    {
        m_live_audio->stop();
    }
    if (m_live_transcriber != nullptr)
    {
        delete m_live_transcriber;
        m_live_transcriber = nullptr;
    }
    if (m_live_audio != nullptr)
    {
        delete m_live_audio;
        m_live_audio = nullptr;
    }

    m_time = 0;
    m_scroll_vel = 0.f;

    Logger::info("done: reset");
}

void PianoRoll::load_audio(std::string_view path)
{
    reset();

    if (m_audio != nullptr)
    {
        pause();
        delete m_audio;
    }
    if (m_transcriber != nullptr)
    {
        delete m_transcriber;
    }

    m_audio = new Audio { path };
}

void PianoRoll::transcribe_audio(TranscriberOptions options)
{
    if (m_audio == nullptr)
    {
        Logger::info("load an audio file first before transcribing");
        return;
    }
    if (m_transcriber != nullptr)
    {
        delete m_transcriber;
    }
    m_transcriber = new Transcriber { };
    m_transcriber->set_options(options);
    m_transcriber->run(m_audio->get_data());
    m_is_ready = true;

    // cool animation after loading
    if (std::abs(m_time) < 1.f)
    {
        m_time = 30.f;
    }
    set_time(0.f);
    set_pixels_per_second(100.f);
}

void PianoRoll::init_live_transcriber(TranscriberOptions options)
{
    reset();

    Logger::info("initializing for live transcription...");

    m_live_audio = new LiveAudio { };

    m_live_transcriber = new LiveTranscriber { m_live_audio };
    m_live_transcriber->set_options(options);

    m_is_ready = true;

    Logger::info("done: initialized for live transcription...");
}

void PianoRoll::start_live_transcriber()
{
    if (m_live_audio == nullptr || m_live_transcriber == nullptr)
    {
        Logger::info("LiveAudio and LiveTranscriber not initialized");
        return;
    }

    // this makes note blocks move upward instead
    if (m_pixels_per_second >= 0.f)
    {
        m_pixels_per_second *= -1.f;
    }

    m_live_audio->start();
    m_live_transcriber->start();
}

void PianoRoll::stop_live_transcriber()
{
    if (m_live_audio == nullptr || m_live_transcriber == nullptr)
    {
        Logger::info("LiveAudio and LiveTranscriber not initialized");
        return;
    }

    m_live_transcriber->stop();
    m_live_audio->stop();
}

unsigned PianoRoll::get_note_index() const
{
    return m_note_index;
}

unsigned PianoRoll::get_note_count() const
{
    if (m_transcriber)
    {
        return m_transcriber->get_note_count();
    }
    if (m_live_transcriber)
    {
        return m_live_transcriber->get_note_count();
    }
    return 0;
}

float PianoRoll::get_time() const
{
    return m_time;
}

float PianoRoll::get_duration() const
{
    if (m_audio)
    {
        return m_audio->get_duration();
    }
    if (m_live_audio)
    {
        return m_live_audio->get_duration();
    }
    return 0.f;
}

float PianoRoll::get_pixels_per_second() const
{
    return m_pixels_per_second;
}

std::string PianoRoll::get_file_name() const
{
    if (m_audio == nullptr)
    {
        return "";
    }
    std::string path { m_audio->get_path() };
    return path.substr(path.find_last_of('/') + 1);
}

Chord PianoRoll::get_chord() const
{
    if (m_transcriber == nullptr)
    {
        return { Letter::none, ChordType::other, -1.f, -1.f, -1.f };
    }
    for (Chord chord : m_transcriber->get_chords())
    {
        if (chord.end_time >= m_time)
        {
            return chord;
        }
    }
    return { Letter::none, ChordType::other, -1.f, -1.f, -1.f };
}

bool PianoRoll::is_playing() const
{
    return m_audio == nullptr ? false : m_audio->is_playing();
}

bool PianoRoll::is_recording() const
{
    return m_live_audio == nullptr ? false : m_live_audio->is_recording();
}

void PianoRoll::set_time(float time)
{
    if (m_audio != nullptr && m_audio->is_playing())
    {
        m_time = std::max(time, 0.f);
        m_audio->pause();
        m_audio->play(m_time);
    }
    else
    {
        // https://www.desmos.com/calculator/pkxj82y9jl
        m_scroll_vel += (time - m_time) / 10.f;
    }
}

void PianoRoll::set_pixels_per_second(float pps)
{
    m_pixels_per_second_vel += (pps - m_pixels_per_second) / 10.f;
}

void PianoRoll::set_max_onset_color(const glm::vec4& color)
{
    m_max_onset_color = color;
}

void PianoRoll::set_min_onset_color(const glm::vec4& color)
{
    m_min_onset_color = color;
}

void PianoRoll::set_bloom_intensity(float intensity)
{
    m_bloom_intensity = intensity;
}

void PianoRoll::play()
{
    if (m_audio == nullptr || m_audio->is_playing())
    {
        return;
    }
    m_audio->play(m_time);
}

void PianoRoll::pause()
{
    if (m_audio == nullptr || !m_audio->is_playing())
    {
        return;
    }
    m_audio->pause();
}

void PianoRoll::render()
{
    if ((m_live_audio && !m_live_audio->is_recording()) || (m_audio && !m_audio->is_playing()))
    {
        if (App::is_key_pressed(GLFW_KEY_LEFT_CONTROL) || App::is_key_pressed(GLFW_KEY_RIGHT_CONTROL))
        {
            if (App::is_key_pressed(GLFW_KEY_UP))
            {
                m_pixels_per_second_vel = +1.5f;
            }
            if (App::is_key_pressed(GLFW_KEY_DOWN))
            {
                m_pixels_per_second_vel = -1.5f;
            }
        }
        else if (App::is_key_pressed(GLFW_KEY_LEFT) || App::is_key_pressed(GLFW_KEY_DOWN))
        {
            m_scroll_vel = -0.2f;
        }
        else if (App::is_key_pressed(GLFW_KEY_RIGHT) || App::is_key_pressed(GLFW_KEY_UP))
        {
            m_scroll_vel = +0.2f;
        }
        m_time += m_scroll_vel;
        m_scroll_vel *= 0.9f;
    }
    else if (m_audio && m_audio->is_playing())
    {
        // a scuffed hack to reduce possibility of a sudden flash of the wrong timestamp when playback begins
        if (std::abs(m_time - m_audio->get_time()) < 1.f)
        {
            m_time = m_audio->get_time();
        }
        m_scroll_vel = 0.f;
    }
    else if (m_live_audio && m_live_audio->is_recording())
    {
        m_time = m_live_audio->get_duration();
    }

    m_pixels_per_second += m_pixels_per_second_vel;
    m_pixels_per_second_vel *= 0.9f;

    m_projection = glm::ortho(0.f, (float)App::get_window_width(), 0.f, (float)App::get_window_height(), -1.f, 1.f);

    m_note_fbo.bind();
    constexpr GLfloat clear_color[] { 0.f, 0.f, 0.f, 0.f };
    glClearBufferfv(GL_COLOR, 0, clear_color);

    // render note blocks

    Transcriber* transcriber { m_transcriber != nullptr ? m_transcriber : m_live_transcriber };

    if (transcriber == nullptr)
    {
        Logger::info("oops something went wrong");
        return;
    }

    std::vector<InstanceData> inst_data;
    inst_data.reserve(transcriber->get_note_count());

    std::vector<glm::vec3> key_colors;
    key_colors.resize(88);

    const float keyboard_top { 0.08f * App::get_window_width() };

    m_note_index = 0;

    for (int key { 0 }; key < 88; key++)
    {
        // dimensions of note block
        float x { get_key_x_pos(key) };
        float width { get_key_width(key) };

        key_colors[key] = is_white_key(key)
            ? glm::vec3(0.92f, 0.92f, 0.92f)
            : glm::vec3(0.06f, 0.06f, 0.06f);

        std::lock_guard<std::mutex> lock { LiveTranscriber::copy_mutex };

        for (const Note& note : transcriber->get_notes()[key]) {
            // more dimensions of note block
            float y { keyboard_top + ((note.start_time - m_time) * m_pixels_per_second) };
            float height { (note.end_time - note.start_time + 0.02f) * m_pixels_per_second };

            // culling, skip notes completely off screen
            if (m_pixels_per_second > 0)
            {
                if (y > App::get_window_height())
                {
                    continue;
                }
                if (y + height < keyboard_top)
                {
                    m_note_index++;
                    continue;
                }
            }
            else
            {
                if (y + height > App::get_window_height())
                {
                    m_note_index++;
                    continue;
                }
                if (y < keyboard_top)
                {
                    continue;
                }
                m_note_index++;
            }

            glm::mat4 model ( 1.0f );
            model = glm::translate(model, glm::vec3(x, y, 0.f));
            model = glm::scale(model, glm::vec3(width, height, 1.f));

            glm::vec4 color { m_min_onset_color + (m_max_onset_color - m_min_onset_color) * note.onset_prob };
            inst_data.emplace_back(model, color);

            if ((m_pixels_per_second > 0 && y < keyboard_top) || (m_pixels_per_second <= 0 && y + height < keyboard_top))
            {
                key_colors[key] = is_white_key(key) ? 1.2f * color : 0.8f * color;
            }
        }
    }

    m_notes_program.use();
    m_notes_program.set_uniform_mat4("projection", glm::value_ptr(m_projection));
    m_notes_vao.bind();
    m_inst_vbo.bind();
    m_inst_vbo.data(inst_data.size() * sizeof(InstanceData), inst_data.data(), GL_DYNAMIC_DRAW);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, inst_data.size());
    m_inst_vbo.unbind();
    m_notes_vao.unbind();

    bool horizontal { true };
    unsigned blurPasses { 12 };

    glViewport(0, 0, App::get_window_width() / 2, App::get_window_height() / 2);

    m_blur_program.use();
    m_quad_vao.bind();
    glActiveTexture(GL_TEXTURE0);

    for (int i { 0 }; i < blurPasses; i++) {
        m_ping_pong_fbos[horizontal].bind();
        m_blur_program.set_uniform_1i("horizontal", horizontal);

        if (i == 0)
        {
            m_note_color_texture.bind();
        }
        else
        {
            m_ping_pong_textures[!horizontal].bind();
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);

        horizontal = !horizontal;
    }

    glViewport(0, 0, App::get_window_width(), App::get_window_height());

    m_note_fbo.unbind();
    glClear(GL_COLOR_BUFFER_BIT);

    m_composite_program.use();
    m_composite_program.set_uniform_1f("bloom_intensity", m_bloom_intensity);

    m_note_color_texture.activate(GL_TEXTURE0);
    m_note_color_texture.bind();
    m_note_color_texture.activate(GL_TEXTURE1);
    m_ping_pong_textures[!horizontal].bind();

    m_quad_vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // render piano keyboard

    m_piano_program.use();
    m_piano_program.set_uniform_mat4("projection", glm::value_ptr(m_projection));
    m_vao.bind();

    for (int key { 0 }; key < 88; key++)
    {
        if (!is_white_key(key))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(get_key_x_pos(key), 0.f, 0.f));
        model = glm::scale(model, glm::vec3(get_white_key_width(), keyboard_top, 1.f));
        m_piano_program.set_uniform_4f("color", key_colors[key].r, key_colors[key].g, key_colors[key].b, 1.f);
        m_piano_program.set_uniform_mat4("model", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    for (int key { 0 }; key < 88; key++)
    {
        if (is_white_key(key))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(get_key_x_pos(key), 0.3f * keyboard_top, 0.f));
        model = glm::scale(model, glm::vec3(get_black_key_width(), 0.7f * keyboard_top, 1.f));
        m_piano_program.set_uniform_4f("color", key_colors[key].r, key_colors[key].g, key_colors[key].b, 1.f);
        m_piano_program.set_uniform_mat4("model", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    m_vao.unbind();
}

float PianoRoll::get_key_x_pos(unsigned key)
{
    Pitch pitch { key_to_pitch(key) };
    float white_key_width { get_white_key_width() + 1.f };
    float black_key_width { get_black_key_width() };
    switch (pitch.letter)
    {
        case Letter::a:     return (7 * pitch.octave) * white_key_width;
        case Letter::b:     return (7 * pitch.octave + 1) * white_key_width;
        case Letter::c:     return (7 * pitch.octave - 5) * white_key_width;
        case Letter::d:     return (7 * pitch.octave - 4) * white_key_width;
        case Letter::e:     return (7 * pitch.octave - 3) * white_key_width;
        case Letter::f:     return (7 * pitch.octave - 2) * white_key_width;
        case Letter::g:     return (7 * pitch.octave - 1) * white_key_width;
        case Letter::as:    return (7 * pitch.octave + 1) * white_key_width - (0.5f * black_key_width);
        case Letter::cs:    return (7 * pitch.octave - 4) * white_key_width - (0.5f * black_key_width);
        case Letter::ds:    return (7 * pitch.octave - 3) * white_key_width - (0.5f * black_key_width);
        case Letter::fs:    return (7 * pitch.octave - 1) * white_key_width - (0.5f * black_key_width);
        case Letter::gs:    return (7 * pitch.octave) * white_key_width - (0.5f * black_key_width);
    }
    return -1.f;
}

bool PianoRoll::is_white_key(unsigned key)
{
    unsigned note { (key + 21) % 12 };
    return note == 0 || note == 2 || note == 4 || note == 5 || note == 7 || note == 9 || note == 11;
}

float PianoRoll::get_key_width(unsigned key)
{
    return is_white_key(key) ? get_white_key_width() : get_black_key_width();
}

float PianoRoll::get_white_key_width()
{
    return App::get_window_width() / 52.f - 1.f;
}

float PianoRoll::get_black_key_width()
{
    return 0.55f * get_white_key_width();
}
