#pragma once

#include "MouseEvent.hpp"
#include "KeyboardEvent.hpp"
#include "IRenderRequestHandler.hpp"

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

    void set_screen_size(size_t w, size_t h)
    {
        m_screen_w = w;
        m_screen_h = h;
    }

protected:
    virtual void on_activated();
    virtual void on_deactivated();

    void request_render();

protected:
    size_t m_screen_w {0};
    size_t m_screen_h {0};
private:
    IRenderRequestHandler* m_render_request_handler{nullptr};
};


} // namespace Slic3r::App::Platform
