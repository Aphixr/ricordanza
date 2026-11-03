#pragma once

#include "window.hpp"
#include "../io/audio.hpp"

class App final
{

public:

    static void init();

    static void run();

private:

    inline static Window window { "Ricordanza", 1280, 720 };

    static Audio audio;

    App() = default;

};

