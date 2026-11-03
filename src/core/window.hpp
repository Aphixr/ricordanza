#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>

class Window
{

public:

    Window(const char* title, unsigned width, unsigned height);

    ~Window();

    bool shouldClose() const;

    void render();

private:

    GLFWwindow* m_window;

    friend class App;

};
