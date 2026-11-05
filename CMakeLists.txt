cmake_minimum_required(VERSION 3.10)
project(Ricordanza)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# prefer GL Vendor-Neutral Dispatch over Legacy
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

set(ONNXRUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/onnxruntime)

include_directories(extern ${ONNXRUNTIME_DIR}/include)

add_executable(Ricordanza
    src/core/app.cpp
    src/core/piano_roll.cpp
    src/core/window.cpp
    src/dsp/analyzer.cpp
    src/gl/buffer.cpp
    src/gl/program.cpp
    src/gl/shader.cpp
    src/gl/vertex_array.cpp
    src/io/audio.cpp
    src/io/file_load_failure_exception.cpp
    src/io/miniaudio.c
    src/io/shader_manager.cpp
    src/ml/transcriber.cpp
    src/util/logger.cpp
    src/main.cpp
    extern/glad/glad.c
    extern/imgui/imgui.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui_impl_glfw.cpp
    extern/imgui/imgui_impl_opengl3.cpp
)

target_link_libraries(Ricordanza
    m pthread dl fftw3f
    glfw OpenGL::GL
    ${ONNXRUNTIME_DIR}/lib/libonnxruntime.so
)
