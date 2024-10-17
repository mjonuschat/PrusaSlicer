set(SLIC3R_PLATFORM_FILES
        include/Slic3r/App/Platform/AbstractRenderModule.hpp
        src/Slic3r/App/Platform/AbstractRenderModule.cpp
        include/Slic3r/App/Platform/AbstractRenderCanvas.hpp
        src/Slic3r/App/Platform/AbstractRenderCanvas.cpp
        include/Slic3r/App/Platform/MouseEvent.hpp
        include/Slic3r/App/Platform/KeyboardEvent.hpp
        include/Slic3r/App/Platform/PlatformError.hpp
        include/Slic3r/App/Platform/KeyModifers.hpp
        include/Slic3r/App/Platform/KeyCode.hpp
        include/Slic3r/App/Platform/IMainThreadDispatcher.hpp
        include/Slic3r/App/Platform/IRenderRequestHandler.hpp
        src/Slic3r/App/Platform/StdMainThreadDispatcher.cpp
        include/Slic3r/App/Platform/StdMainThreadDispatcher.hpp
        src/Slic3r/App/Platform/PlatformServices.cpp
        include/Slic3r/App/Platform/PlatformServices.hpp
        src/Slic3r/App/Platform/JobManager.cpp
        include/Slic3r/App/Platform/JobManager.hpp
)


add_library(slic3r-platform STATIC ${SLIC3R_PLATFORM_FILES})
target_include_directories(slic3r-platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(slic3r-platform PUBLIC slic3r-base slic3r-render libslic3r)


# Doxygen
add_doxygen_target(slic3r-platform)


