#include "Slic3r/App/Platform/AbstractRenderCanvas.hpp"

#include <iostream>
#include <algorithm>

#include <imgui/imgui.h>
#if !USE_IMGUI_RENDER
#include <imgui/backends/imgui_impl_opengl3.h>
#else
#include <Slic3r/App/Render/Context.hpp>
#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Render/CommandBuffer.hpp>
#include <Slic3r/App/Render/Geometry.hpp>
#include <Slic3r/App/Render/Texture.hpp>
#endif
#include <GL/glew.h>
#include <Slic3r/Log.hpp>


#ifdef NDEBUG
#define assert_no_gl_error()
#else
#define assert_no_gl_error() { GLenum err = glGetError(); assert(err == GL_NO_ERROR);}
#endif

void imgui_rendered_fallback_glyph(ImWchar c)
{
    // TODO: implement glyph loading (postponed)
}


namespace Slic3r::App::Platform {


void AbstractRenderCanvas::set_render_module(AbstractRenderModule* render_module)
{
    if (m_render_module)
        m_render_module->deactivate();
    m_render_module = render_module;
    if (m_render_module) {
        m_render_module->set_screen_size(m_screen_info);
        m_render_module->activate(this);
    }
}

void AbstractRenderCanvas::set_screen_size(const ScreenInfo& screen_info)
{
    m_screen_info = screen_info;
    if (m_render_module)
        m_render_module->set_screen_size(m_screen_info);
}

void AbstractRenderCanvas::render()
{
    if (m_render_module == nullptr)
        return;


    m_render_module->ensure_initialized(get_device());

#if USE_IMGUI_RENDER
    if (!m_imgui_render) {
        m_imgui_render = std::make_unique<Render::ImguiRender>(Render::Context::instance().device());
    }
#endif

    assert_no_gl_error();
    begin_frame();
    assert_no_gl_error();
    begin_imgui_frame();
    assert_no_gl_error();
    m_render_module->render_imgui();
    assert_no_gl_error();
    end_imgui_frame();
    assert_no_gl_error();
    emit_enqueued_events();
    assert_no_gl_error();
    m_render_module->render_scene();
    assert_no_gl_error();
    end_frame();
}



void AbstractRenderCanvas::begin_frame()
{
    begin_frame_platform();
    assert_no_gl_error();

    ImGuiIO& io = ImGui::GetIO();

    double current_time = get_platform_time();
    io.DeltaTime = m_last_time > 0 ? float(current_time - m_last_time) : (1.0f / 60.0f);
    m_last_time = current_time;

    /*
    assert_no_gl_error();
    glViewport(0, 0, m_screen_info.physical_width(), m_screen_info.physical_height());
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
    */
}

void AbstractRenderCanvas::begin_imgui_frame()
{
    // Start the Dear ImGui frame
#if USE_IMGUI_RENDER
    m_imgui_render->new_frame();
#else
    ImGui_ImplOpenGL3_NewFrame();
#endif
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

#if USE_IMGUI_RENDER
    const ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data) {
        auto& dev = Render::Context::instance().device();
        auto buffer = dev.create_command_buffer();
        m_imgui_render->render(*buffer, draw_data);
        buffer->submit();
    }
#else
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data)
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
#endif
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
    m_render_request_count += 2;
}

bool AbstractRenderCanvas::get_and_reset_render_requested()
{
    if (m_render_request_count > 0) {
        m_render_request_count--;
        return true;
    }
    return false;
}

} // namespace Slic3r::App::Platform
