#include "app.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

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

    piano_roll.load_audio("contemplation.wav");
}

int App::get_window_width()
{
    return window_width;
}

int App::get_window_height()
{
    return window_height;
}

void App::on_render(const std::function<void()>& callback)
{
    on_render_callbacks.push_back(callback);
}

void App::run()
{
    while (!window.shouldClose())
    {
        Logger::info("running");

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i { 0 }; i < on_render_callbacks.size(); i++)
        {
            on_render_callbacks[i]();
        }

        window.run();
    }

    std::cout << std::endl;
}
