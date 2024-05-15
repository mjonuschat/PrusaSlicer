set(SLIC3R_APP_SDLTEST_FILES
        ./src/main.cpp
)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(CMAKE_EXE_LINKER_FLAGS -sPTHREAD_POOL_SIZE=16)
endif()
add_executable(slic3r-app-sdltest ${SLIC3R_APP_SDLTEST_FILES})
target_link_libraries(slic3r-app-sdltest PUBLIC slic3r-platform slic3r-platform-sdl slic3r-shared)
