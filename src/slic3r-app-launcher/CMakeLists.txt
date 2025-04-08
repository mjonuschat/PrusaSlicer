set(SLIC3R_APP_LAUNCHER_FILES
        src/Slic3r/App/Launcher/main.cpp
        src/Slic3r/App/Launcher/ReadCLI.cpp
        src/Slic3r/App/Launcher/ReadCLI.hpp
)

# Create a slic3r-app-launcher executable
# Process mainfests for various platforms.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/slic3r-app-launcher.rc.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/PrusaSlicer.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.manifest @ONLY)

add_executable(slic3r-app-launcher ${SLIC3R_APP_LAUNCHER_FILES}
                                   ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.rc)

# Following is the no-console version.
#add_executable(slic3r-app-launcher WIN32 ${SLIC3R_APP_LAUNCHER_FILES}
#                                         ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.rc)


set_target_properties(slic3r-app-launcher PROPERTIES OUTPUT_NAME "slic3r-app-desktop")

target_link_libraries(slic3r-app-launcher PUBLIC
        slic3r-app-desktop
        slic3r-app-cli
)


# TEMPORARY, BEFORE ALL CMAKES ARE UPDATED:
target_compile_definitions(slic3r-app-launcher PUBLIC SLIC3R_GUI=1)


if (MSVC)
    # Manifest is provided through PrusaSlicer.rc, don't generate your own.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif ()

slic3r_link_resources(slic3r-app-launcher)

if (WIN32)
    prusaslicer_copy_dlls(slic3r-app-launcher)
endif ()

install(TARGETS slic3r-app-launcher RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
