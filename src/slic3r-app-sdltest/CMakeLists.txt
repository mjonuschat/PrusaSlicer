set(SLIC3R_APP_SDLTEST_FILES
        ./src/main.cpp
)

add_executable(slic3r-app-sdltest WIN32 ${SLIC3R_APP_SDLTEST_FILES})
target_link_libraries(slic3r-app-sdltest PUBLIC slic3r-platform slic3r-platform-sdl slic3r-shared)
