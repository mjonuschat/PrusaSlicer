set(SLIC3R_APP_DESKTOP_FILES
        src/Slic3r/App/Desktop/DesktopApp.cpp
        src/Slic3r/App/Desktop/DesktopApp.hpp
        src/Slic3r/App/Desktop/MainFrame.cpp
        src/Slic3r/App/Desktop/MainFrame.hpp
)

add_executable(slic3r-app-desktop WIN32 ${SLIC3R_APP_DESKTOP_FILES})

target_link_libraries(slic3r-app-desktop PUBLIC
        slic3r-platform slic3r-platform-wx slic3r-shared
        ${wxWidgets_LIBRARIES} NanoSVG::nanosvg NanoSVG::nanosvgrast
)
