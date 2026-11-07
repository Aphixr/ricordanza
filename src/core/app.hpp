#pragma once

#include <functional>
#include <vector>
#include <unordered_set>
#include <utility>

#include "window.hpp"
#include "piano_roll.hpp"

class App final
{

public:

    static void init();

    static int get_window_width();

    static int get_window_height();

    static void on_render(const std::function<void ()>& callback);

    static void on_key_down(int glfw_key_code, const std::function<void ()>& callback);

    static void on_scroll(const std::function<void (double x_offset, double y_offset)>& callback);

    static void on_drop(const std::function<void (int count, const char** paths)>& callback);

    static bool is_key_pressed(int glfw_key_code);

    static void run();

private:

    inline static unsigned window_width { 1280 };

    inline static unsigned window_height { 720 };

    inline static Window window { "Ricordanza", window_width, window_height };

    inline static std::vector<std::function<void ()>> on_render_callbacks { };

    inline static std::vector<std::function<void (double, double)>> on_scroll_callbacks { };

    inline static std::vector<std::pair<int, std::function<void ()>>> on_key_down_callbacks { };

    inline static std::vector<std::function<void (int, const char**)>> on_drop_callbacks { };

    inline static PianoRoll piano_roll { };

    static void window_size_callback(GLFWwindow*, int width, int height);

    static void key_down_callback(GLFWwindow*, int key, int scancode, int action, int mods);

    static void scroll_callback(GLFWwindow*, double x_offset, double y_offset);

    static void drop_callback(GLFWwindow*, int count, const char** paths);

    App() = default;

};
