set(SLIC3R3_APP_SOURCES
        main.cpp
        main.hpp
        App.hpp
        App.cpp
        View/Frame.cpp
        View/Frame.hpp
        View/IPanel.hpp
        View/TestFrame.cpp
        View/TestFrame.hpp
        View/FrameNavigable.hpp
)
add_library(slic3r3_app ${SLIC3R3_APP_SOURCES})
target_link_libraries(slic3r3_app PUBLIC imgui imgui_backend libslic3r)
