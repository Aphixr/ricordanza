#include "piano_roll.hpp"

#include "app.hpp"
#include "../gl/shader.hpp"
#include "../util/pitch.hpp"

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
    , m_time { 0.f }
    , m_shader_program { }
    , m_vao { }
    , m_vbo { GL_ARRAY_BUFFER }
    , m_projection { glm::ortho(0.f, (float)App::get_window_width(), 0.f, (float)App::get_window_height(), -1.f, 1.f) }
{
    m_shader_program.attach(gl::ShaderType::Vertex, vert_src);
    m_shader_program.attach(gl::ShaderType::Fragment, frag_src);
    m_shader_program.link();

    m_vao.bind();
    m_vbo.bind();
    m_vbo.data(sizeof(vertices), vertices, GL_STATIC_DRAW);
    m_vao.attribute(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    m_vao.enable_attribute_array(0);
    m_vao.unbind();
    m_vbo.unbind();

    App::on_render([this]()
    {
        this->render();
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
}

float PianoRoll::get_time() const
{
    return m_time;
}

void PianoRoll::render()
{
    m_shader_program.use();
    m_vao.bind();

    for (int i { 21 }; i <= 108; i++)
    {
        if (!is_white_key(i))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(midi_pitch_to_horizontal_position(i), 2.f, 0.f));
        model = glm::scale(model, glm::vec3(get_white_key_width() - 1.f, 100.f, 1.f));
        m_shader_program.set_uniform_4f("color", 0.9f, 0.9f, 0.9f, 1.f);
        m_shader_program.set_uniform_mat4("projection", glm::value_ptr(m_projection));
        m_shader_program.set_uniform_mat4("model", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    for (int i { 21 }; i <= 108; i++)
    {
        if (is_white_key(i))
        {
            continue;
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(midi_pitch_to_horizontal_position(i), 42.f, 0.f));
        model = glm::scale(model, glm::vec3(get_black_key_width(), 64.f, 1.f));
        m_shader_program.set_uniform_4f("color", 0.06f, 0.06f, 0.06f, 1.f);
        m_shader_program.set_uniform_mat4("projection", glm::value_ptr(m_projection));
        m_shader_program.set_uniform_mat4("model", glm::value_ptr(model));
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
