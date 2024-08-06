set(SLIC3R_PLATFORM_FILES
        src/Slic3r/App/Platform/AbstractRenderModule.hpp
        src/Slic3r/App/Platform/AbstractRenderModule.cpp
        src/Slic3r/App/Platform/AbstractRenderCanvas.hpp
        src/Slic3r/App/Platform/AbstractRenderCanvas.cpp
        src/Slic3r/App/Platform/MouseEvent.hpp
        src/Slic3r/App/Platform/KeyboardEvent.hpp
        src/Slic3r/App/Platform/PlatformError.hpp
        src/Slic3r/App/Platform/KeyModifers.hpp
        src/Slic3r/App/Platform/KeyCode.hpp
        src/Slic3r/App/Platform/IMainThreadDispatcher.hpp
        src/Slic3r/App/Platform/IRenderRequestHandler.hpp
        src/Slic3r/App/Platform/StdMainThreadDispatcher.cpp
        src/Slic3r/App/Platform/StdMainThreadDispatcher.hpp
        src/Slic3r/App/Platform/PlatformServices.cpp
        src/Slic3r/App/Platform/PlatformServices.hpp
        src/Slic3r/App/Platform/JobManager.cpp
        src/Slic3r/App/Platform/JobManager.hpp
        src/Slic3r/App/Platform/PlatformInfo.hpp
        src/Slic3r/App/Platform/PlatformInfo.cpp
        src/Slic3r/App/Platform/ScreenInfo.hpp
)

if (APPLE)
    set(SLIC3R_PLATFORM_FILES
            ${SLIC3R_PLATFORM_FILES}
            src/Slic3r/App/Platform/PlatformInfo_osx.mm
    )
elseif (WIN32)
    set(SLIC3R_PLATFORM_FILES
            ${SLIC3R_PLATFORM_FILES}
            src/Slic3r/App/Platform/PlatformInfo_win32.cpp
    )
elseif (EMSCRIPTEN)
    set(SLIC3R_PLATFORM_FILES
            ${SLIC3R_PLATFORM_FILES}
            src/Slic3r/App/Platform/PlatformInfo_emscripten.cpp
    )
elseif (UNIX)
    set(SLIC3R_PLATFORM_FILES
            ${SLIC3R_PLATFORM_FILES}
            src/Slic3r/App/Platform/PlatformInfo_unix.cpp
    )
endif ()

add_library(slic3r-platform STATIC ${SLIC3R_PLATFORM_FILES})
target_include_directories(slic3r-platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(slic3r-platform PUBLIC PlatformGL imgui imgui_backend_ogl3 slic3r-base slic3r-shared libslic3r)


