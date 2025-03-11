set(SLIC3R_PLATFORM_WX_FILES
        include/Slic3r/App/Platform/WX/WXRenderCanvas.hpp
        src/Slic3r/App/Platform/WX/WXRenderCanvas.cpp
        include/Slic3r/App/Platform/WX/WXMainThreadDispatcher.hpp
        src/Slic3r/App/Platform/WX/WXMainThreadDispatcher.cpp
)

add_library(slic3r-platform-wx STATIC ${SLIC3R_PLATFORM_WX_FILES})

if(WIN32)
    message(STATUS "WXWIN environment set to: $ENV{WXWIN}")
elseif(UNIX)
    set(wxWidgets_USE_UNICODE ON)
    if(SLIC3R_STATIC)
        set(wxWidgets_USE_STATIC ON)
    else()
        set(wxWidgets_USE_STATIC OFF)
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set (wxWidgets_CONFIG_OPTIONS "--toolkit=gtk${SLIC3R_GTK}")
endif ()
find_package(NanoSVG REQUIRED)
find_package(wxWidgets 3.2 CONFIG REQUIRED COMPONENTS base core adv html gl webview)

slic3r_remap_configs(wx::wxhtml wx::wxadv wx::wxgl wx::wxcore wx::wxbase RelWithDebInfo Release)

target_link_libraries(slic3r-platform-wx PUBLIC
        wxWidgets::wxWidgets
        OpenGL::GL
        GLEW::GLEW
        imgui_backend_ogl3
        slic3r-platform
        slic3r-shared
)
target_include_directories(slic3r-platform-wx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)


# Doxygen
add_target_to_doxygen(slic3r-platform-wx)
