#include "piano_roll.hpp"

#include <vector>

#include "app.hpp"
#include "../io/shader_manager.hpp"
#include "../util/pitch.hpp"
#include "../util/time.hpp"
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

struct InstanceData
{
    glm::mat4 model;
    glm::vec4 color;
};

PianoRoll::PianoRoll()
    : m_audio { nullptr }
    , m_transcriber { nullptr }
    , m_time { 0.f }
    , m_scroll_vel { 0.f }
    , m_piano_program { }
    , m_notes_program { }
    , m_vao { }
    , m_vbo { GL_ARRAY_BUFFER }
    , m_notes_vbo { GL_ARRAY_BUFFER }
    , m_inst_vbo { GL_ARRAY_BUFFER }
    , m_projection { glm::ortho(0.f, (float)App::get_window_width(), 0.f, (float)App::get_window_height(), -1.f, 1.f) }
{
    m_piano_program.attach(ShaderManager::get("../res/default.vert.glsl"));
    m_piano_program.attach(ShaderManager::get("../res/solid.frag.glsl"));
    m_piano_program.link();

    m_vao.bind();
    m_vbo.bind();
    m_vbo.data(sizeof(vertices), vertices, GL_STATIC_DRAW);
    m_vao.attribute(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    m_vao.enable_attribute_array(0);
    m_vao.unbind();
    m_vbo.unbind();

    m_notes_program.attach(ShaderManager::get("../res/instance.vert.glsl"));
    m_notes_program.attach(ShaderManager::get("../res/instance.frag.glsl"));
    m_notes_program.link();

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

    App::on_render([this]()
    {
        render();
    });
}

PianoRoll::~PianoRoll()
{
    delete m_audio;
    delete m_transcriber;
    m_audio = nullptr;
    m_transcriber = nullptr;
}

void PianoRoll::load_audio(std::string_view path)
{
    if (m_audio != nullptr)
    {
        delete m_audio;
    }
    if (m_transcriber != nullptr)
    {
        delete m_transcriber;
    }

    m_audio = new Audio { path };
    m_transcriber = new Transcriber { "basic_pitch.onnx" };
    m_transcriber->run(m_audio->get_data());

    App::on_key_down(GLFW_KEY_SPACE, [this]()
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
    });

    App::on_scroll([this](double x_offset, double y_offset)
    {
        if (App::is_key_pressed(GLFW_KEY_LEFT_CONTROL) || App::is_key_pressed(GLFW_KEY_RIGHT_CONTROL))
        {
            m_pixels_per_second = std::max(std::min<float>(m_pixels_per_second - 2.f * y_offset, 1000.f), 5.f);
        }
        else
        {
            m_scroll_vel = 1.2f * y_offset * (100.f / m_pixels_per_second);
        }
    });
}

float PianoRoll::get_time() const
{
    return m_time;
}

void PianoRoll::render()
{
    if (!m_audio->is_playing())
    {
        if (App::is_key_pressed(GLFW_KEY_LEFT) || App::is_key_pressed(GLFW_KEY_DOWN))
        {
            m_scroll_vel = -0.2f;
        }
        if (App::is_key_pressed(GLFW_KEY_RIGHT) || App::is_key_pressed(GLFW_KEY_UP))
        {
            m_scroll_vel = +0.2f;
        }
        m_time += m_scroll_vel;
        m_scroll_vel *= 0.9f;
    }
    else
    {
        // a scuffed hack to reduce possibility of a sudden flash of the wrong timestamp when playback begins
        if (std::abs(m_time - m_audio->get_time()) < 1.f)
        {
            m_time = m_audio->get_time();
        }
        m_scroll_vel = 0.f;
    }
    Logger::update("timestamp"s, "timestamp: " + seconds_to_string(m_time) + " / " + seconds_to_string(m_audio->get_duration()));

    m_projection = glm::ortho(0.f, (float)App::get_window_width(), 0.f, (float)App::get_window_height(), -1.f, 1.f);

    // render note blocks

    std::vector<InstanceData> inst_data;
    inst_data.reserve(m_transcriber->get_note_count());

    std::vector<glm::vec3> key_colors;
    key_colors.resize(88);

    const float keyboard_top { 0.08f * App::get_window_width() };

    for (int key { 0 }; key < 88; key++)
    {
        // dimensions of note block
        float x { midi_pitch_to_horizontal_position(key + 21) };
        float width { is_white_key(key + 21) ? get_white_key_width() - 1 : get_black_key_width() };

        key_colors[key] = is_white_key(key + 21)
            ? glm::vec3(0.92f, 0.92f, 0.92f)
            : glm::vec3(0.06f, 0.06f, 0.06f);

        for (const Note& note : m_transcriber->get_notes()[key]) {
            // more dimensions of note block
            float y { keyboard_top + ((note.start_time - m_time) * m_pixels_per_second) };
            float height { (note.end_time - note.start_time + 0.02f) * m_pixels_per_second };

            // culling, skip notes completely off screen
            if (y > App::get_window_height() || y + height < keyboard_top)
            {
                continue;
            }

            glm::vec3 color { 0.1f + note.onset_prob, 0.1f + (0.3f - 0.3f * note.onset_prob), 1.1f - note.onset_prob };
            glm::mat4 model ( 1.0f );
            model = glm::translate(model, glm::vec3(x, y, 0.f));
            model = glm::scale(model, glm::vec3(width - 1.f, height, 1.f));
            inst_data.emplace_back(model, glm::vec4(color, 1.f));

            if (y < keyboard_top)
            {
                key_colors[key] = is_white_key(key + 21) ? 1.2f * color : 0.8f * color;
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

    // render piano keyboard

    m_piano_program.use();
    m_piano_program.set_uniform_mat4("projection", glm::value_ptr(m_projection));
    m_vao.bind();

    for (int i { 21 }; i <= 108; i++)
    {
        const int key { i - 21 };
        if (!is_white_key(i))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(midi_pitch_to_horizontal_position(i), 0.f, 0.f));
        model = glm::scale(model, glm::vec3(get_white_key_width() - 1.f, keyboard_top, 1.f));
        m_piano_program.set_uniform_4f("color", key_colors[key].r, key_colors[key].g, key_colors[key].b, 1.f);
        m_piano_program.set_uniform_mat4("model", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    for (int i { 21 }; i <= 108; i++)
    {
        const int key { i - 21 };
        if (is_white_key(i))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(midi_pitch_to_horizontal_position(i), 0.3f * keyboard_top, 0.f));
        model = glm::scale(model, glm::vec3(get_black_key_width(), 0.7f * keyboard_top, 1.f));
        m_piano_program.set_uniform_4f("color", key_colors[key].r, key_colors[key].g, key_colors[key].b, 1.f);
        m_piano_program.set_uniform_mat4("model", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    m_vao.unbind();
}

float PianoRoll::midi_pitch_to_horizontal_position(unsigned midi)
{
    Pitch pitch { midi_to_pitch(midi) };
    float white_key_width { get_white_key_width() };
    float black_key_width { get_black_key_width() };
    switch (pitch.letter)
    {
        case Letter::A:     return (7 * pitch.octave) * white_key_width;
        case Letter::B:     return (7 * pitch.octave + 1) * white_key_width;
        case Letter::C:     return (7 * pitch.octave - 5) * white_key_width;
        case Letter::D:     return (7 * pitch.octave - 4) * white_key_width;
        case Letter::E:     return (7 * pitch.octave - 3) * white_key_width;
        case Letter::F:     return (7 * pitch.octave - 2) * white_key_width;
        case Letter::G:     return (7 * pitch.octave - 1) * white_key_width;
        case Letter::As:    return (7 * pitch.octave + 1) * white_key_width - (0.5f * black_key_width);
        case Letter::Cs:    return (7 * pitch.octave - 4) * white_key_width - (0.5f * black_key_width);
        case Letter::Ds:    return (7 * pitch.octave - 3) * white_key_width - (0.5f * black_key_width);
        case Letter::Fs:    return (7 * pitch.octave - 1) * white_key_width - (0.5f * black_key_width);
        case Letter::Gs:    return (7 * pitch.octave) * white_key_width - (0.5f * black_key_width);
    }
    return -1.f;
}

bool PianoRoll::is_white_key(unsigned midi)
{
    unsigned note { midi % 12 };
    return note == 0 || note == 2 || note == 4 || note == 5 || note == 7 || note == 9 || note == 11;
}

float PianoRoll::get_white_key_width()
{
    return App::get_window_width() / 52.f;
}

float PianoRoll::get_black_key_width()
{
    return 0.55f * get_white_key_width();
}
