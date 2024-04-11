set(SLIC3R3_APP_SOURCES
        main.cpp
        main.hpp
        App.hpp
        App.cpp
        View/BaseRenderModule.hpp
        View/TestRenderModule.cpp
        View/TestRenderModule.hpp
        Plater/Scene/Scene.cpp
        Plater/Scene/Scene.hpp
        Presenter/ScenePresenter.cpp
        Presenter/ScenePresenter.hpp
        Renderer/Device.cpp
        Renderer/Device.hpp
        View/IView.hpp
        View/TestView.cpp
        View/TestView.hpp
        View/BaseRenderModule.cpp
        Platform/IRenderingPlatform.hpp
        Platform/RenderingPlatformImpl.cpp
        Platform/RenderingPlatformImpl.hpp
)


add_library(slic3r3_app ${SLIC3R3_APP_SOURCES})

option(SLIC3R3_SDL_ENABLED "Enable SDL backend" 1)

target_link_libraries(slic3r3_app imgui libslic3r)

if (SLIC3R3_SDL_ENABLED)
    target_link_libraries(slic3r3_app imgui_backend_ogl3_sdl2)
    add_compile_definitions(slic3r3_app -DSLIC3R3_SDL_ENABLED)
else()
    target_link_libraries(slic3r3_app imgui_backend_ogl3_glfw)
endif()


    include(CMakePrintHelpers)
cmake_print_properties(TARGETS slic3r3_app imgui imgui_backend_ogl3_glfw libslic3r PROPERTIES LINK_LIBRARIES LINK_INTERFACE_LIBRARIES LINK_FLAGS)
