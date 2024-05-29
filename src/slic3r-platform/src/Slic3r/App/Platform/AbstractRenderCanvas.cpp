#include "AbstractRenderCanvas.hpp"

#include <iostream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <gl/glew.h>
#include <spdlog/spdlog.h>

#ifdef NDEBUG
#define assert_no_gl_error()
#else
#define assert_no_gl_error() { GLenum err = glGetError(); assert(err == GL_NO_ERROR);}
#endif

namespace Slic3r::App::Platform {

void AbstractRenderCanvas::set_render_module(AbstractRenderModule* render_module)
{
    if (m_render_module)
        m_render_module->deactivate();
    m_render_module = render_module;
    if (m_render_module) {
        m_render_module->set_screen_size(m_screen_w, m_screen_h);
        m_render_module->activate(this);
    }
}

void AbstractRenderCanvas::set_screen_size(size_t w, size_t h)
{
    m_screen_w = w;
    m_screen_h = h;
    if (m_render_module)
        m_render_module->set_screen_size(w, h);
}

void AbstractRenderCanvas::render()
{
    if (m_render_module == nullptr)
        return;

    SPDLOG_INFO("AbstractRenderCanvas::render 1a");
    assert_no_gl_error();
    SPDLOG_INFO("AbstractRenderCanvas::render 1b");
    begin_frame();
    SPDLOG_INFO("AbstractRenderCanvas::render 2");
    assert_no_gl_error();
    begin_imgui_frame();
    SPDLOG_INFO("AbstractRenderCanvas::render 3");
    assert_no_gl_error();
    m_render_module->render_imgui();
    SPDLOG_INFO("AbstractRenderCanvas::render 4");
    assert_no_gl_error();
    end_imgui_frame();
    SPDLOG_INFO("AbstractRenderCanvas::render 5");
    assert_no_gl_error();
    emit_enqueued_events();
    SPDLOG_INFO("AbstractRenderCanvas::render 6");
    assert_no_gl_error();
    m_render_module->render_scene();
    SPDLOG_INFO("AbstractRenderCanvas::render 7");
    assert_no_gl_error();
    end_frame();
    SPDLOG_INFO("AbstractRenderCanvas::render 8");
}



void AbstractRenderCanvas::begin_frame()
{
    begin_frame_platform();
    assert_no_gl_error();

    ImGuiIO& io = ImGui::GetIO();

    double current_time = get_platform_time();
    io.DeltaTime = m_last_time > 0 ? float(current_time - m_last_time) : (1.0f / 60.0f);
    m_last_time = current_time;

    assert_no_gl_error();
    glViewport(0, 0, m_screen_w, m_screen_h);
    assert_no_gl_error();
    // TODO: this should be render module responsibility
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(
        clear_color.x * clear_color.w, clear_color.y * clear_color.w,
        clear_color.z * clear_color.w, clear_color.w
    );
    assert_no_gl_error();
    glClear(GL_COLOR_BUFFER_BIT);
    assert_no_gl_error();
}

void AbstractRenderCanvas::begin_imgui_frame()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");
    begin_imgui_frame_platform();
    ImGui::NewFrame();
}

void AbstractRenderCanvas::end_imgui_frame()
{
    end_imgui_frame_platform();
    // Rendering
    ImGui::Render();

}

void AbstractRenderCanvas::end_frame()
{
    ImDrawData *draw_data = ImGui::GetDrawData();
    if (draw_data)
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    end_frame_platform();
}

void AbstractRenderCanvas::update_key_modifiers(KeyboardEvent::Type event_type, KeyCode code)
{
    KeyModifiers mods{KeyModifiers(KeyModifier::None)};

    if (is_alt(code)) {
        mods |= KeyModifiers(KeyModifier::Alt);
    }
    if (is_shift(code)) {
        mods |= KeyModifiers(KeyModifier::Shift);
    }
    if (is_ctrl(code)) {
        mods |= KeyModifiers(KeyModifier::Ctrl);
    }
    if (is_meta(code)) {
        mods |= KeyModifiers(KeyModifier::Meta);
    }

    if (event_type == KeyboardEvent::Type::KeyUp) {
        m_key_modifiers = m_key_modifiers & ~mods;
    } else if (event_type == KeyboardEvent::Type::KeyDown) {
        m_key_modifiers = m_key_modifiers | mods;
    }
}

void AbstractRenderCanvas::update_mouse_position(int x, int y)
{
    m_mouse_x = x;
    m_mouse_y = y;
}

void AbstractRenderCanvas::enqueue_mouse(const MouseEvent& e)
{
    m_enqueued_mouse_events.push_back(e);
}
void AbstractRenderCanvas::enqueue_keyboard(const KeyboardEvent& e)
{
    m_enqueued_keyboard_events.push_back(e);
}

void AbstractRenderCanvas::emit_enqueued_events()
{
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        for (const auto& e : m_enqueued_keyboard_events)
            emit_keyboard(e);
    }
    m_enqueued_keyboard_events.clear();

    if (!io.WantCaptureMouse) {
        for (const auto& e : m_enqueued_mouse_events)
            emit_mouse(e);
    } else if (std::any_of(io.MouseDown, io.MouseDown + 5, [](bool val){ return val; })){
        request_render();
    }
    m_enqueued_mouse_events.clear();
}


void AbstractRenderCanvas::request_render()
{
    m_render_requested = true;
}

bool AbstractRenderCanvas::get_and_reset_render_requested()
{
    const bool ret = m_render_requested;
    if (m_render_requested)
        m_render_requested = false;
    return ret;
}

} // namespace Slic3r::App::Platform
