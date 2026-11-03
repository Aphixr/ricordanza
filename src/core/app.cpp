#include "app.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "../util/logger.hpp"

Audio App::audio { "./contemplation.wav" };

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
}

void App::run()
{
    while (!window.shouldClose())
    {
        Logger::info("running");

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        window.run();
    }

    std::cout << std::endl;
}
