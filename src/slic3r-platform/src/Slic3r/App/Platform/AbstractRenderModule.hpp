#pragma once

#include "MouseEvent.hpp"
#include "KeyboardEvent.hpp"
#include "IRenderRequestHandler.hpp"
#include "ScreenInfo.hpp"

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

    void set_screen_size(const ScreenInfo& screen_info) { m_screen_info = screen_info; }

protected:
    virtual void on_activated();
    virtual void on_deactivated();

    void request_render();

protected:
    ScreenInfo m_screen_info {0, 0, 1};
private:
    IRenderRequestHandler* m_render_request_handler{nullptr};
};


} // namespace Slic3r::App::Platform
