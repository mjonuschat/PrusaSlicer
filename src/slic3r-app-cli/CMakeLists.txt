set(SLIC3R_APP_CLI_FILES
        src/Slic3r/App/CLI/CLIApp.cpp
        include/Slic3r/App/CLI/CLIApp.hpp
)


add_library(slic3r-app-cli ${SLIC3R_APP_CLI_FILES} )

target_link_libraries(slic3r-app-cli PUBLIC
        slic3r-base
        slic3r-platform 
        slic3r-shared
)

target_include_directories(slic3r-app-cli PUBLIC include)

install(TARGETS slic3r-app-cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
