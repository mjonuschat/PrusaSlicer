set(SLIC3R_PLATFORM_SDL_FILES
        ./src/Slic3r/App/Platform/SDL/SDLRenderCanvas.cpp
        ./src/Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp
)

add_library(slic3r-platform-sdl STATIC ${SLIC3R_PLATFORM_SDL_FILES})
find_package(SDL2 2 REQUIRED)
target_link_libraries(slic3r-platform-sdl PUBLIC
        SDL2::SDL2
        OpenGL::GL
        GLEW::GLEW
        imgui_backend_ogl3
        slic3r-platform
)
target_include_directories(slic3r-platform-sdl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
