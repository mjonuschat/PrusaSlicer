set(SLIC3R_APP_WXTEST_FILES
        src/Slic3r/App/WXTest/TestApp.cpp
        src/Slic3r/App/WXTest/TestApp.hpp
        src/Slic3r/App/WXTest/MainFrame.cpp
        src/Slic3r/App/WXTest/MainFrame.hpp
)

if (WIN32)
    add_executable(slic3r-app-wxtest WIN32 ${SLIC3R_APP_WXTEST_FILES})
else()
    add_executable(slic3r-app-wxtest ${SLIC3R_APP_WXTEST_FILES})
endif()

target_link_libraries(slic3r-app-wxtest PUBLIC
        slic3r-platform slic3r-platform-wx slic3r-shared
        ${wxWidgets_LIBRARIES} NanoSVG::nanosvg NanoSVG::nanosvgrast
)


SLIC3R_LINK_RESOURCES(slic3r-app-wxtest)
