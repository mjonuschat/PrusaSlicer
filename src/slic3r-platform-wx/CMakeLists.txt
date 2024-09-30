set(SLIC3R_PLATFORM_WX_FILES
        include/Slic3r/App/Platform/WX/WXRenderCanvas.hpp
        src/Slic3r/App/Platform/WX/WXRenderCanvas.cpp
)

add_library(slic3r-platform-wx STATIC ${SLIC3R_PLATFORM_WX_FILES})

target_link_libraries(slic3r-platform-wx PUBLIC
        ${wxWidgets_LIBRARIES}
        OpenGL::GL
        GLEW::GLEW
        imgui_backend_ogl3
        slic3r-platform
        slic3r-shared
)
target_include_directories(slic3r-platform-wx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
