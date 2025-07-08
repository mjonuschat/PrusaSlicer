if(SLIC3R_BUILD_TESTS AND NOT EMSCRIPTEN)
    find_package(Catch2 3.8 REQUIRED)
    slic3r_remap_configs("Catch2::Catch2" RelWithDebInfo Release)

    set(CATCH_EXTRA_ARGS "" CACHE STRING "Extra arguments for Catch2 test suites.")
    set(CATCH_ALL_ARGS "exclude:[NotWorking]" "exclude:[Slow]")
    list(APPEND CATCH_ALL_ARGS ${CATCH_EXTRA_ARGS})
endif()

add_subdirectory(slic3r-base)
add_subdirectory(slic3r-domain)
add_subdirectory(slic3r-biz-algorithms)
add_subdirectory(slic3r-biz-cgal-algorithms)
add_subdirectory(slic3r-biz-parser)
add_subdirectory(slic3r-gcode-reader)
add_subdirectory(libpgcode)

add_subdirectory(slic3r-jthread)
add_subdirectory(libslic3r)
add_subdirectory(libvgcode)
add_subdirectory(slic3r-biz-arrange)
add_subdirectory(slic3r-arrange)
add_subdirectory(slic3r-arrange-wrapper)
#add_subdirectory(libseqarrange)
add_subdirectory(slic3r-app-cli)

if (SLIC3R_ENABLE_FORMAT_STEP)
    add_subdirectory(occt_wrapper)
endif ()

if (EMSCRIPTEN)
    add_subdirectory(imgui)
    add_subdirectory(libvgcode)
endif()

if (SLIC3R_GUI)
    add_subdirectory(slic3r-platform-wx)
    add_subdirectory(slic3r-app-desktop)
    add_subdirectory(slic3r-render)
    add_subdirectory(slic3r-platform)
    add_subdirectory(slic3r-platform-sdl)
    add_subdirectory(slic3r-shared)
    add_subdirectory(slic3r-shared-wx)
    add_subdirectory(slic3r-app-sdltest)
endif ()

add_subdirectory(slic3r-app-launcher)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/osx/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)

add_doxygen_target(slic3r-doxygen
    EXTRA_INPUT ../doc/Doxygen.md ../doc/CodeStyle.md
)
