#include "app.hpp"

#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include "../util/time.hpp"

#include "../util/logger.hpp"

void App::init()
{
    // for some reason, if you put this ImGui initialization code at the end of Window's constructor,
    // it results in ImGuiIO& ImGui::GetIO(ImGuiContext*): Assertion `ctx != __null' failed
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window.m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glfwSetWindowSizeCallback(window.m_window, window_size_callback);
    glfwSetKeyCallback(window.m_window, key_down_callback);
    glfwSetScrollCallback(window.m_window, scroll_callback);
    glfwSetDropCallback(window.m_window, drop_callback);

    on_drop([=](int count, const char** paths)
    {
        std::string path { paths[0] };
        if (path.ends_with(".mp3") || path.ends_with(".wav") || path.ends_with(".flac"))
        {
            piano_roll.load_audio(paths[0]);
        }
    });
}

int App::get_window_width()
{
    return window_width;
}

int App::get_window_height()
{
    return window_height;
}

void App::on_window_resize(const std::function<void (int width, int height)>& callback)
{
    on_window_resize_callbacks.push_back(callback);
}

void App::on_render(const std::function<void()>& callback)
{
    on_render_callbacks.push_back(callback);
}

void App::on_key_down(int key_code, const std::function<void()>& callback)
{
    on_key_down_callbacks.push_back({ key_code, callback });
}

void App::on_scroll(const std::function<void (double x_offset, double y_offset)>& callback)
{
    on_scroll_callbacks.push_back(callback);
}

void App::on_drop(const std::function<void (int count, const char** paths)>& callback)
{
    on_drop_callbacks.push_back(callback);
}

bool App::is_key_pressed(int key_code)
{
    return glfwGetKey(window.m_window, key_code) == GLFW_PRESS;
}

void App::run()
{
    while (!window.shouldClose())
    {
        glClearColor(0.133f, 0.133f, 0.133f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (std::function<void ()>& callback : on_render_callbacks)
        {
            callback();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTabBar("Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Load"))
            {
                ImGui::Text("%s",
                    piano_roll.get_file_name() != ""
                    ? ("File: " + piano_roll.get_file_name()).c_str()
                    : "Drag and drop an audio file (.mp3, .wav, .flac) to load it."
                );
                ImGui::Checkbox("Connect broken notes", &state_connect_broken_notes);
                ImGui::Checkbox("Filter out harmonic overtones", &state_filter_overtones);
                ImGui::Checkbox("Filter out blips", &state_filter_blips);
                if (ImGui::Button("Transcribe"))
                {
                    piano_roll.transcribe_audio({ state_connect_broken_notes, state_filter_overtones, state_filter_blips });
                    piano_roll.set_time(0.f);
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("View"))
            {
                if (ImGui::Button("Play / Pause"))
                {
                    if (piano_roll.is_playing())
                    {
                        piano_roll.pause();
                    }
                    else
                    {
                        piano_roll.play();
                    }
                }
                
                if (ImGui::ArrowButton("-5 seconds", ImGuiDir_Left))
                {
                    piano_roll.set_time(piano_roll.get_time() - 5.f);
                }
                ImGui::SameLine();
                if (ImGui::ArrowButton("+5 seconds", ImGuiDir_Right))
                {
                    piano_roll.set_time(piano_roll.get_time() + 5.f);
                }
                ImGui::SameLine();
                ImGui::Text("%s / %s (%d / %d notes)",
                    seconds_to_string(piano_roll.get_time()).c_str(),
                    seconds_to_string(piano_roll.get_duration()).c_str(),
                    piano_roll.get_note_index(),
                    piano_roll.get_note_count()
                );

                if (ImGui::ArrowButton("-15\% pps", ImGuiDir_Left))
                {
                    piano_roll.set_pixels_per_second(piano_roll.get_pixels_per_second() - 15.f);
                }
                ImGui::SameLine();
                if (ImGui::ArrowButton("+15\% pps", ImGuiDir_Right))
                {
                    piano_roll.set_pixels_per_second(piano_roll.get_pixels_per_second() + 15.f);
                }
                ImGui::SameLine();
                ImGui::Text("%d pixels per second", (int)piano_roll.get_pixels_per_second());

                ImGui::PushItemWidth(90.f);

                ImGui::SliderFloat("##minr", &state_min_onset_color[0], -0.2f, 1.2f, "%.2f");
                ImGui::SameLine();
                ImGui::SliderFloat("##ming", &state_min_onset_color[1], -0.2f, 1.2f, "%.2f");
                ImGui::SameLine();
                ImGui::SliderFloat("##minb", &state_min_onset_color[2], -0.2f, 1.2f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Min onset color");
                piano_roll.set_min_onset_color(state_min_onset_color);

                ImGui::SliderFloat("##maxr", &state_max_onset_color[0], -0.2f, 1.2f, "%.2f");
                ImGui::SameLine();
                ImGui::SliderFloat("##maxg", &state_max_onset_color[1], -0.2f, 1.2f, "%.2f");
                ImGui::SameLine();
                ImGui::SliderFloat("##maxb", &state_max_onset_color[2], -0.2f, 1.2f, "%.2f");
                ImGui::SameLine();
                ImGui::Text("Max onset color");
                piano_roll.set_max_onset_color(state_max_onset_color);

                ImGui::PopItemWidth();

                ImGui::Text("Chord: %s %s (confidence: %.3f)",
                    key_to_letter_string((int)piano_roll.get_chord().root).c_str(),
                    chord_type_to_string(piano_roll.get_chord().type).c_str(),
                    piano_roll.get_chord().prob
                );

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Log"))
            {
                ImGui::Text("%s", Logger::get_log().c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.run();
    }

    std::cout << std::endl;
}

void App::window_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
    window_width = width;
    window_height = height;
    
    for (std::function<void (int, int)>& callback : on_window_resize_callbacks)
    {
        callback(width, height);
    }
}

void App::key_down_callback(GLFWwindow*, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        for (std::pair<int, std::function<void ()>>& entry : on_key_down_callbacks)
        {
            if (entry.first == key)
            {
                entry.second();
            }
        }
    }
}

void App::scroll_callback(GLFWwindow*, double x_offset, double y_offset)
{
    for (std::function<void (double, double)>& callback : on_scroll_callbacks)
    {
        callback(x_offset, y_offset);
    }
}

void App::drop_callback(GLFWwindow*, int count, const char** paths)
{
    for (std::function<void (int, const char**)>& callback : on_drop_callbacks)
    {
        callback(count, paths);
    }
}
