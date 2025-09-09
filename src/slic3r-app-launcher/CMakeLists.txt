# Create a slic3r-app-launcher executable
# Process mainfests for various platforms.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/slic3r-app-launcher.rc.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/PrusaSlicer.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.manifest @ONLY)

add_executable(slic3r-app-launcher
    src/Slic3r/App/Launcher/main.cpp
    src/Slic3r/App/Launcher/ReadCLI.cpp
    src/Slic3r/App/Launcher/ReadCLI.hpp
    src/Slic3r/App/Launcher/SentryScope.hpp
    src/Slic3r/App/Launcher/SentryScope.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-launcher.rc
)

if (MSVC)
target_link_options(slic3r-app-launcher PRIVATE "$<$<CONFIG:RELEASE>:/DEBUG>")
endif ()

target_link_libraries(slic3r-app-launcher PRIVATE slic3r-app-cli)

# sentry integration
if (SLIC3R_SENTRY)
    find_package(sentry REQUIRED COMPONENTS crashpad)
    target_compile_definitions(slic3r-app-launcher PRIVATE
        SLIC3R_SENTRY
        "SLIC3R_SENTRY_DSN=\"${SLIC3R_SENTRY_DSN}\""
    )

    target_link_libraries(slic3r-app-launcher PRIVATE sentry::sentry)
    if(TARGET sentry_crashpad::crashpad_handler)
        message(STATUS "SENTRY crashpad handler target present")
    endif ()

    add_custom_command(
            TARGET slic3r-app-launcher POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:sentry_crashpad::crashpad_handler>"
            "$<TARGET_FILE_DIR:slic3r-app-launcher>"
            COMMENT "Copying crashpad_handler to build directory"
    )
endif (SLIC3R_SENTRY)
#end of sentry integration part

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
