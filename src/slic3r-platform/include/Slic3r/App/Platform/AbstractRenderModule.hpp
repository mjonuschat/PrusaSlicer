#pragma once

#include "Slic3r/App/Platform/MouseEvent.hpp"
#include "Slic3r/App/Platform/KeyboardEvent.hpp"
#include "Slic3r/Biz/Platform/IRenderRequestHandler.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"
#include "Slic3r/App/Platform/CommandRegistry.hpp"

namespace Slic3r::App::Render {
class Device;
class CommandBuffer;
class ImguiRender;
}

namespace Slic3r::App::Platform {


/**
 * Provides abstract interface for render module and common infrastructure for rendering
 * and event processing.
 */
class AbstractRenderModule
{
public:
    virtual ~AbstractRenderModule() = default;

    virtual void render_scene() = 0;
    virtual void render_imgui() = 0;

    virtual void on_scene_mouse_event(const MouseEvent& e);
    virtual void on_scene_keyboard_event(const KeyboardEvent& e);

    void activate(Biz::Platform::IRenderRequestHandler* render_request_handler);
    void deactivate();

    void set_screen_size(const Render::ScreenInfo& screen_info);
    void ensure_initialized(Render::Device& device)
    {
        if (!m_initialized) {
            on_init(device);
            register_commands();
            m_initialized = true;
        }
    }

    void set_imgui_render(Render::ImguiRender* imgui_render);

protected:
    /**
     * Initialize all Render objects here.
     */
    virtual void on_init(Render::Device& device) { m_device = &device; }

    virtual void on_activated();
    virtual void on_deactivated();
    virtual void on_screen_resized();
    virtual void on_set_imgui_render() {}

    virtual void register_commands() {}
    void request_render();

protected:
    Render::Device* m_device{nullptr};
    CommandRegistry m_command_registry;
    Render::ImguiRender* m_imgui_render{nullptr};

    Render::ScreenInfo m_screen_info {0, 0, 1};
    bool m_initialized{false};
private:
    Biz::Platform::IRenderRequestHandler* m_render_request_handler{nullptr};
};


} // namespace Slic3r::App::Platform
