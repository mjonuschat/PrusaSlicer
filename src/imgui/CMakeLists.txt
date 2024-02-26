cmake_minimum_required(VERSION 2.8.12)
project(imgui)

add_library(imgui STATIC
    imconfig.h
    imgui.cpp
    imgui.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_stdlib.cpp
    imgui_stdlib.h
    imgui_tables.cpp
    imgui_widgets.cpp
    # imgui STB
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
)

add_library(imgui_backend STATIC
        backends/imgui_impl_glfw.h
        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_opengl3.h
        backends/imgui_impl_opengl3.cpp
)

target_include_directories(
        imgui_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(
        imgui_backend PUBLIC glfw3 OpenGL::GL GLEW::GLEW
)

if (APPLE)
    target_link_libraries(imgui_backend PUBLIC "-framework Cocoa" "-framework IOKit")
endif ()
