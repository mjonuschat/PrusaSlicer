set(SLIC3R_APP_SDLTEST_FILES
        ./src/main.cpp
)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(CMAKE_EXE_LINKER_FLAGS "--preload-file ${SLIC3R_RESOURCES_NEXT_DIR}@/resources -sPTHREAD_POOL_SIZE=16 -fwasm-exceptions ${CMAKE_EXE_LINKER_FLAGS}")
    message(STATUS "CMake set CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
endif()

add_executable(slic3r-app-sdltest WIN32 ${SLIC3R_APP_SDLTEST_FILES})

target_link_libraries(slic3r-app-sdltest PUBLIC slic3r-platform slic3r-platform-sdl slic3r-shared fmt slic3r-base)


slic3r_link_resources(slic3r-app-sdltest)
slic3r_msvc_set_deps_env_path(slic3r-app-sdltest)
if (WIN32)
    prusaslicer_copy_dlls(slic3r-app-sdltest)
endif ()
