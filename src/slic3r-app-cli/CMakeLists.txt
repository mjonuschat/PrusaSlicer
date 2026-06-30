set(SLIC3R_APP_CLI_FILES
        src/Slic3r/App/CLI/CLIApp.cpp
        include/Slic3r/App/CLI/CLIApp.hpp
        src/Slic3r/App/CLI/CLIRuntime.cpp
        include/Slic3r/App/CLI/CLIRuntime.hpp
        src/Slic3r/App/CLI/CLIUtils.cpp
        include/Slic3r/App/CLI/CLIUtils.hpp
        src/Slic3r/App/CLI/ProcessTransform.cpp
        include/Slic3r/App/CLI/ProcessTransform.hpp
        src/Slic3r/App/CLI/ProcessActions.cpp
        include/Slic3r/App/CLI/ProcessActions.hpp
        src/Slic3r/App/CLI/LoadPrintData.cpp
        include/Slic3r/App/CLI/LoadPrintData.hpp
        src/Slic3r/App/CLI/ProfilesSharingUtils.cpp
        include/Slic3r/App/CLI/ProfilesSharingUtils.hpp
        src/Slic3r/App/CLI/ProfilePresetSelection.cpp
        include/Slic3r/App/CLI/ProfilePresetSelection.hpp
)


add_library(slic3r-app-cli ${SLIC3R_APP_CLI_FILES})

target_link_libraries(slic3r-app-cli PUBLIC
        slic3r-shared
        slic3r-biz-arrange
)

target_include_directories(slic3r-app-cli PUBLIC include)

# TODO: This temporarily allows using private header files from libslic3r in CLI.
target_include_directories(slic3r-app-cli PUBLIC "$<TARGET_PROPERTY:libslic3r,SOURCE_DIR>/src")

install(TARGETS slic3r-app-cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
