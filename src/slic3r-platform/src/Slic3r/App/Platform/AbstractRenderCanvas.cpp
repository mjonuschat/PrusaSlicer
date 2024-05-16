#include "AbstractRenderCanvas.hpp"

#include <iostream>
#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <gl/glew.h>

namespace Slic3r::App::Platform {

void AbstractRenderCanvas::set_render_module(AbstractRenderModule* render_module)
{
    if (m_render_module)
        m_render_module->deactivate();
    m_render_module = render_module;
    if (m_render_module)
        m_render_module->activate(this);
}

void AbstractRenderCanvas::render()
{
    if (m_render_module == nullptr)
        return;

    begin_frame();
    begin_imgui_frame();
    m_render_module->render_imgui();
    end_imgui_frame();
    emit_enqueued_events();
    m_render_module->render_scene();
    end_frame();
}



void AbstractRenderCanvas::begin_frame()
{
    ImGuiIO& io = ImGui::GetIO();

    double current_time = get_platform_time();
    io.DeltaTime = m_last_time > 0 ? float(current_time - m_last_time) : (1.0f / 60.0f);
    m_last_time = current_time;

    int display_w, display_h;
    display_w = io.DisplaySize.x;
    display_h = io.DisplaySize.y;
    glViewport(0, 0, display_w, display_h);

    // TODO: this should be render module responsibility
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(
        clear_color.x * clear_color.w, clear_color.y * clear_color.w,
        clear_color.z * clear_color.w, clear_color.w
    );
    glClear(GL_COLOR_BUFFER_BIT);

    begin_frame_platform();

}

void AbstractRenderCanvas::begin_imgui_frame()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
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
