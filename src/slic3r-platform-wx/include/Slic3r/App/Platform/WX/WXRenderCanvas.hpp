#pragma once

#include <memory>
#include <chrono>

#include <GL/glew.h>
#include <wx/glcanvas.h>

#include "Slic3r/App/Platform/AbstractRenderCanvas.hpp"


namespace Slic3r::App::Platform::WX {

class WXRenderCanvas : public Platform::AbstractRenderCanvas, public wxGLCanvas
{
public:
    WXRenderCanvas(wxWindow* parent);
    ~WXRenderCanvas();

    WXRenderCanvas(const WXRenderCanvas&) = delete;
    WXRenderCanvas operator=(const WXRenderCanvas&) = delete;

    void render() override;
    void dispatch_on_main_thread(Biz::Platform::IMainThreadDispatcher::Function  func);

protected:
    void begin_frame_platform() override;
    void begin_imgui_frame_platform()override;
    void end_imgui_frame_platform() override;
    void end_frame_platform() override;
    double platform_time() override;
    Render::Device& device() override;

private:
    void on_paint(wxPaintEvent& event);
    void on_size(wxSizeEvent& event);
    void on_keyboard(wxKeyEvent&evt);
    void on_mouse(wxMouseEvent& event);
    void on_mouse_enter(wxMouseEvent& event);
    void on_mouse_leave(wxMouseEvent& event);
    void on_idle(wxIdleEvent& event);

    static KeyModifiers modifiers(const wxKeyboardState& event);

    void init();
    void init_wx_imgui();

private:
    using Clock = std::chrono::high_resolution_clock;
    std::unique_ptr<wxGLContext> m_gl_context;
    std::chrono::time_point<Clock> m_start_time;

    std::string m_glsl_version;
    bool m_initialized{false};
};

}
