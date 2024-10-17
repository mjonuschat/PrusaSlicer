#include "Slic3r/App/Platform/AbstractRenderModule.hpp"

namespace Slic3r::App::Platform {

void AbstractRenderModule::on_scene_mouse_event(const MouseEvent& e) {}
void AbstractRenderModule::on_scene_keyboard_event(const KeyboardEvent& e) {}
void AbstractRenderModule::on_activated() {}
void AbstractRenderModule::on_deactivated() {}
void AbstractRenderModule::on_screen_resized() {}



void AbstractRenderModule::activate(IRenderRequestHandler* render_request_handler)
{
    m_render_request_handler = render_request_handler;
    on_activated();
}

void AbstractRenderModule::deactivate()
{
    on_deactivated();
    m_render_request_handler = nullptr;
}

void AbstractRenderModule::request_render()
{
    m_render_request_handler->request_render();
}

void AbstractRenderModule::set_screen_size(const Render::ScreenInfo& screen_info)
{
    if (m_screen_info != screen_info) {
        m_screen_info = screen_info;
        on_screen_resized();
    }
}


} // namespace Slic3r::App::Platform

