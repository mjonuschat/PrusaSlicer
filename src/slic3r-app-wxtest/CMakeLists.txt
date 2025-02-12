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

find_package(NanoSVG REQUIRED)

target_link_libraries(slic3r-app-wxtest PUBLIC
        slic3r-base slic3r-platform slic3r-platform-wx slic3r-shared
)


slic3r_link_resources(slic3r-app-wxtest)
if (WIN32)
    prusaslicer_copy_dlls(slic3r-app-wxtest)
endif ()
