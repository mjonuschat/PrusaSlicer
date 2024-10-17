#pragma once

#include "Slic3r/App/Platform/MouseEvent.hpp"
#include "Slic3r/App/Platform/KeyboardEvent.hpp"
#include "Slic3r/App/Platform/IRenderRequestHandler.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"

namespace Slic3r::App::Render {
class Device;
class CommandBuffer;
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

    void activate(IRenderRequestHandler* render_request_handler);
    void deactivate();

    void set_screen_size(const Render::ScreenInfo& screen_info);
    void ensure_initialized(Render::Device& device)
    {
        if (!m_initialized) {
            on_init(device);
            m_initialized = true;
        }
    }


protected:
    /**
     * Initialize all Render objects here.
     */
    virtual void on_init(Render::Device& device) { m_device = &device; }

    virtual void on_activated();
    virtual void on_deactivated();
    virtual void on_screen_resized();

    void request_render();

protected:
    Render::Device* m_device{nullptr};

    Render::ScreenInfo m_screen_info {0, 0, 1};
    bool m_initialized{false};
private:
    IRenderRequestHandler* m_render_request_handler{nullptr};
};


} // namespace Slic3r::App::Platform
