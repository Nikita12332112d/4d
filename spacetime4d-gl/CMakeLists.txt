cmake_minimum_required(VERSION 3.15)
project(spacetime4d_gl CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(spacetime4d-gl WIN32
    src/main.cpp
    src/glcore.cpp
    src/gfx.cpp
    src/text.cpp
    src/scene.cpp
)

target_link_libraries(spacetime4d-gl PRIVATE opengl32 gdi32 user32)

if (MSVC)
    target_compile_options(spacetime4d-gl PRIVATE /W4 /O2)
    target_compile_definitions(spacetime4d-gl PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(spacetime4d-gl PRIVATE -Wall -Wextra -O2)
endif()
