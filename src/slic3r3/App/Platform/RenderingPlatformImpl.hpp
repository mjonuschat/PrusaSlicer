#pragma once

#include "IRenderingPlatform.hpp"

#include <memory>
#include <GL/glew.h>
#include "imgui/imgui.h"
#ifdef SLIC3R3_SDL_ENABLED
#include "imgui/backends/imgui_impl_sdl.h"
#else
#include <imgui/backends/imgui_impl_glfw.h>
#endif
#include "imgui/backends/imgui_impl_opengl3.h"

#include <iostream>
#ifdef SLIC3R3_SDL_ENABLED
#include <SDL.h>
#else
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#endif // SLIC3R3_SDL_ENABLED


namespace Slic3r::App::Platform {
class RenderingPlatformImpl : public IRenderingPlatform
{
#ifdef SLIC3R3_SDL_ENABLED
    static void glfw_error_callback(int error, const char *description);
#endif // SLIC3R3_SDL_ENABLED

public:
    ~RenderingPlatformImpl() override ;
    bool init() override;
    bool should_quit() override;
    void poll_events() override;
    void begin_imgui_frame() override;
    void end_imgui_frame() override;
    void begin_frame() override;
    void end_frame() override;

private:
#ifdef SLIC3R3_SDL_ENABLED
    bool m_should_quit{false};
    SDL_Window* m_window{nullptr};
    SDL_GLContext m_gl_context{nullptr};
#else
    GLFWwindow* m_window{nullptr};
#endif
};

}  // namespace Slic3r::App