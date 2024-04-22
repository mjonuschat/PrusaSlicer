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

protected:
    virtual void on_activated();
    virtual void on_deactivated();
private:
    IRenderRequestHandler* m_render_request_handler{nullptr};
};


} // namespace Slic3r::App::Platform
