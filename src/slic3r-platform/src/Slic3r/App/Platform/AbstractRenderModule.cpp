#include "Slic3r/App/Platform/AbstractRenderModule.hpp"

namespace Slic3r::App::Platform {

void AbstractRenderModule::on_scene_mouse_event(const MouseEvent& e) {}
void AbstractRenderModule::on_scene_keyboard_event(const KeyboardEvent& e)
{
    m_command_registry.process_keyboard_event(e);
}
void AbstractRenderModule::on_activated() {}
void AbstractRenderModule::on_deactivated() {}
void AbstractRenderModule::on_screen_resized() {}

using Biz::Platform::IRenderRequestHandler;

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
    // Only request render when activated
    if (m_render_request_handler)
        m_render_request_handler->request_render();
}

void AbstractRenderModule::set_screen_size(const Render::ScreenInfo& screen_info)
{
    if (m_screen_info != screen_info) {
        m_screen_info = screen_info;
        on_screen_resized();
    }
}

void AbstractRenderModule::ensure_initialized(Render::Device& device, Render::ImguiRender& imgui_render,
    Platform::AnimationManager& animation_manager)
{
    if (!m_initialized) {
        on_init(device, imgui_render, animation_manager);
        register_commands();
        m_initialized = true;
    }
}

void AbstractRenderModule::set_imgui_render(Render::ImguiRender* imgui_render)
{
    if (m_imgui_render != imgui_render) {
        m_imgui_render = imgui_render;
        on_set_imgui_render();
    }
}

void AbstractRenderModule::on_init(Render::Device &device, Render::ImguiRender &imgui_render,
    Platform::AnimationManager& animation_manager)
{
    m_device = &device;
    m_imgui_render = &imgui_render;
    m_animation_manager = &animation_manager;
}

} // namespace Slic3r::App::Platform

