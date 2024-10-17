set(SLIC3R_PLATFORM_SDL_FILES
        ./src/Slic3r/App/Platform/SDL/SDLRenderCanvas.cpp
        ./include/Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp
)

add_library(slic3r-platform-sdl STATIC ${SLIC3R_PLATFORM_SDL_FILES})
if (EMSCRIPTEN)
    # Emscripten provided SDL2 port has no version specified
    find_package(SDL2 REQUIRED)
else ()
    find_package(SDL2 2 REQUIRED)
endif ()
target_link_libraries(slic3r-platform-sdl PUBLIC
        SDL2::SDL2
        PlatformGL
        imgui_backend_ogl3
        slic3r-platform
        slic3r-shared
)
target_include_directories(slic3r-platform-sdl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)


# Doxygen
add_doxygen_target(slic3r-platform-sdl)
