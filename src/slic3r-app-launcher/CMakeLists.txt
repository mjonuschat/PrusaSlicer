# Create a slic3r-app-launcher executable
# Process mainfests for various platforms.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/slic3r-app-launcher.rc.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/PrusaSlicer.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.manifest @ONLY)

add_executable(slic3r-app-launcher
    src/Slic3r/App/Launcher/main.cpp
    src/Slic3r/App/Launcher/ReadCLI.cpp
    src/Slic3r/App/Launcher/ReadCLI.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.rc
)

target_link_libraries(slic3r-app-launcher PRIVATE slic3r-app-cli)
if (SLIC3R_GUI)
    target_link_libraries(slic3r-app-launcher PRIVATE slic3r-app-desktop)
    target_compile_definitions(slic3r-app-launcher PRIVATE SLIC3R_GUI)
endif()

if (MSVC)
    # Manifest is provided through PrusaSlicer.rc, don't generate your own.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif ()

slic3r_link_resources(slic3r-app-launcher ${SLIC3R_RESOURCES_DIR})

if (WIN32)
    prusaslicer_copy_dlls(slic3r-app-launcher)
endif ()

install(TARGETS slic3r-app-launcher RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
