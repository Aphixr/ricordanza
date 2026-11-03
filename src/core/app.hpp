#pragma once

#include <functional>
#include <vector>

#include "window.hpp"
#include "piano_roll.hpp"

class App final
{

public:

    static void init();

    static int get_window_width();

    static int get_window_height();

    static void on_render(const std::function<void()>& callback);

    static void run();

private:

    inline static unsigned window_width { 1280 };

    inline static unsigned window_height { 720 };

    inline static Window window { "Ricordanza", window_width, window_height };

    inline static std::vector<std::function<void()>> on_render_callbacks;

    inline static PianoRoll piano_roll { };

    App() = default;

};

