set(SLIC3R_PLATFORM_FILES
        ./src/Slic3r/App/Platform/AbstractRenderModule.hpp
        ./src/Slic3r/App/Platform/AbstractRenderModule.cpp
        ./src/Slic3r/App/Platform/AbstractRenderCanvas.hpp
        ./src/Slic3r/App/Platform/AbstractRenderCanvas.cpp
        ./src/Slic3r/App/Platform/MouseEvent.hpp
        ./src/Slic3r/App/Platform/KeyboardEvent.hpp
        src/Slic3r/App/Platform/PlatformError.hpp
        src/Slic3r/App/Platform/KeyModifers.hpp
        src/Slic3r/App/Platform/KeyCode.hpp
)

add_library(slic3r-platform STATIC ${SLIC3R_PLATFORM_FILES})
target_include_directories(slic3r-platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(slic3r-platform PUBLIC OpenGL::GL GLEW::GLEW imgui imgui_backend_ogl3)
