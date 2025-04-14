set(SLIC3R_APP_SDLTEST_FILES
    ./src/main.cpp
)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(CMAKE_EXE_LINKER_FLAGS "--preload-file ${SLIC3R_RESOURCES_DIR}@/resources -sPTHREAD_POOL_SIZE=16 -fwasm-exceptions ${CMAKE_EXE_LINKER_FLAGS}")
    message(STATUS "CMake set CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
endif()

add_executable(slic3r-app-sdltest WIN32 ${SLIC3R_APP_SDLTEST_FILES})

target_link_libraries(slic3r-app-sdltest PUBLIC slic3r-platform-sdl fmt)

slic3r_link_resources(slic3r-app-sdltest ${SLIC3R_RESOURCES_DIR})

if (MSVC)
    set_target_properties(slic3r-app-sdltest
        PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${CMAKE_PREFIX_PATH}\\bin"
    )
endif()

if (WIN32)
    prusaslicer_copy_dlls(slic3r-app-sdltest)
endif ()
