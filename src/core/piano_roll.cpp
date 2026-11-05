#include "piano_roll.hpp"

#include <vector>

#include "app.hpp"
#include "../gl/shader.hpp"
#include "../util/pitch.hpp"
#include "../ml/transcriber.hpp"

using namespace std::literals;

const char* vert_src =
R"glsl(
    #version 330 core
    layout (location = 0) in vec2 a_pos;
    uniform mat4 model;
    uniform mat4 projection;
    void main()
    {
        gl_Position = projection * model * vec4(a_pos, 0.0, 1.0);
    }
)glsl";

const char* vert_inst_src =
R"glsl(
    #version 330 core
    layout (location = 0) in vec2 a_pos;
    // layout (location = 1) in vec4 a_inst_box; // x, y, width, height
    layout (location = 1) in mat4 a_model;
    uniform mat4 projection;
    void main()
    {
        // gl_Position = projection * vec4((a_pos * vec2(10, 10)) + a_inst_box.xy, 0.0, 1.0);
        gl_Position = projection * a_model * vec4(a_pos, 0.0, 1.0);
    }
)glsl";

const char* frag_src =
R"glsl(
    #version 330 core
    out vec4 frag_color;
    uniform vec4 color;
    void main()
    {
        frag_color = color;
    }
)glsl";

float vertices[]
{
    0.f, 1.f,
    1.f, 0.f,
    0.f, 0.f,
    0.f, 1.f,
    1.f, 1.f,
    1.f, 0.f
};

PianoRoll::PianoRoll()
    : m_audio { nullptr }
    , m_notes { }
    , m_time { 30.f }
    , m_scroll_vel { 0.f }
    , m_piano_program { }
    , m_notes_program { }
    , m_vao { }
    , m_vbo { GL_ARRAY_BUFFER }
    , m_notes_vbo { GL_ARRAY_BUFFER }
    , m_inst_vbo { GL_ARRAY_BUFFER }
    , m_projection { glm::ortho(0.f, (float)App::get_window_width(), 0.f, (float)App::get_window_height(), -1.f, 1.f) }
{
    m_piano_program.attach(gl::ShaderType::Vertex, vert_src);
    m_piano_program.attach(gl::ShaderType::Fragment, frag_src);
    m_piano_program.link();

    m_vao.bind();
    m_vbo.bind();
    m_vbo.data(sizeof(vertices), vertices, GL_STATIC_DRAW);
    m_vao.attribute(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    m_vao.enable_attribute_array(0);
    m_vao.unbind();
    m_vbo.unbind();

    m_notes_program.attach(gl::ShaderType::Vertex, vert_inst_src);
    m_notes_program.attach(gl::ShaderType::Fragment, frag_src);
    m_notes_program.link();

    m_notes_vao.bind();
    m_notes_vbo.bind();
    m_notes_vbo.data(sizeof(vertices), vertices, GL_STATIC_DRAW);
    m_notes_vao.attribute(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    m_notes_vao.enable_attribute_array(0);
    m_inst_vbo.bind();
    for (int i = 1; i < 5; i++)
    {
        m_notes_vao.enable_attribute_array(i);
        m_notes_vao.attribute(i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)((i - 1) * sizeof(glm::vec4)));
        m_notes_vao.attribute_divisor(i, 1);
    }
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
    m_audio = nullptr;
}

void PianoRoll::load_audio(std::string_view path)
{
    if (m_audio != nullptr)
    {
        delete m_audio;
    }
    
    m_audio = new Audio(path);

    Transcriber transcriber { "basic_pitch.onnx" };
    m_notes = transcriber.run(m_audio->get_data());

    Logger::info("transcribed raw data into vector of "s + std::to_string(m_notes.size()) + " notes"s);

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
            Logger::update("timestamp", "timestamp: "s + std::to_string(m_time));
        }
        if (App::is_key_pressed(GLFW_KEY_RIGHT) || App::is_key_pressed(GLFW_KEY_UP))
        {
            m_scroll_vel = +0.2f;
            Logger::update("timestamp", "timestamp: "s + std::to_string(m_time));
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

    m_projection = glm::ortho(0.f, (float)App::get_window_width(), 0.f, (float)App::get_window_height(), -1.f, 1.f);

    // render note blocks

    // std::vector<glm::vec4> inst_data;
    std::vector<glm::mat4> inst_data;
    inst_data.reserve(m_notes.size());

    const float keyboard_y { 0.14f * App::get_window_height() };

    for (const Note& note : m_notes) {
        if (note.midi_pitch < 21 || note.midi_pitch > 108)
        {
            continue;
        }

        // dimensions of note block
        float x { midi_pitch_to_horizontal_position(note.midi_pitch) };
        float y { keyboard_y + ((note.start_time - m_time) * m_pixels_per_second) };
        float width { is_white_key(note.midi_pitch) ? get_white_key_width() - 1 : get_black_key_width() };
        float height { (note.end_time - note.start_time) * m_pixels_per_second };

        // culling, skip notes completely off screen
        if (y > App::get_window_height() || y + height < keyboard_y)
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x + 1.f, y, 0.f));
        model = glm::scale(model, glm::vec3(width - 2.f, height, 1.f));
        inst_data.push_back(model);
    }

    m_notes_program.use();
    m_notes_program.set_uniform_mat4("projection", glm::value_ptr(m_projection));
    m_notes_program.set_uniform_4f("color", 0.9f, 0.7f, 0.1f, 0.8f);
    m_notes_vao.bind();
    m_inst_vbo.bind();
    // m_notes_vbo.data(inst_data.size() * sizeof(glm::vec4), inst_data.data(), GL_DYNAMIC_DRAW);
    m_inst_vbo.data(inst_data.size() * sizeof(glm::mat4), inst_data.data(), GL_STATIC_DRAW);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, inst_data.size());
    m_inst_vbo.unbind();
    m_notes_vao.unbind();

    // render piano keyboard

    m_piano_program.use();
    m_piano_program.set_uniform_mat4("projection", glm::value_ptr(m_projection));
    m_vao.bind();

    for (int i { 21 }; i <= 108; i++)
    {
        if (!is_white_key(i))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(midi_pitch_to_horizontal_position(i), 0.f, 0.f));
        model = glm::scale(model, glm::vec3(get_white_key_width() - 1.f, 0.14f * App::get_window_height(), 1.f));
        m_piano_program.set_uniform_4f("color", 0.9f, 0.9f, 0.9f, 1.f);
        m_piano_program.set_uniform_mat4("model", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    for (int i { 21 }; i <= 108; i++)
    {
        if (is_white_key(i))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(midi_pitch_to_horizontal_position(i), 0.04f * App::get_window_height(), 0.f));
        model = glm::scale(model, glm::vec3(get_black_key_width(), 0.1f * App::get_window_height(), 1.f));
        m_piano_program.set_uniform_4f("color", 0.06f, 0.06f, 0.06f, 1.f);
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
