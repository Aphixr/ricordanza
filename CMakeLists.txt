cmake_minimum_required(VERSION 3.10)
project(Ricordanza)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(extern)

add_executable(Ricordanza
    src/io/audio.cpp
    src/io/file_load_failure_exception.cpp
    src/io/miniaudio.c
    src/util/logger.cpp
    src/main.cpp
)

target_link_libraries(Ricordanza m pthread dl fftw3f)
