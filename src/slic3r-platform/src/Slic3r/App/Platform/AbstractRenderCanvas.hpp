#pragma once

#include "AbstractRenderModule.hpp"

namespace Slic3r::App::Platform {

/**
 * Abstract class for platform-specific render canvas.
 *
 * Key responsibilities:
 * - facilitate rendering of render module
 * - translate platform specific events and push them the render module
 */
class AbstractRenderCanvas
{
public:
    virtual ~AbstractRenderCanvas() = default;

    void render();
    void set_render_module(AbstractRenderModule* render_module) { m_render_module = render_module; }

protected:
    virtual void begin_frame_platform() = 0;
    virtual void begin_imgui_frame_platform() = 0;
    virtual void end_imgui_frame_platform() = 0;
    virtual void end_frame_platform() = 0;
    virtual double get_platform_time() = 0;

    virtual void emit_mouse(const MouseEvent& e)
    {
        if (m_render_module)
            m_render_module->on_scene_mouse_event(e);
    }

    virtual void emit_keyboard(const KeyboardEvent& e)
    {
        if (m_render_module)
            m_render_module->on_scene_keyboard_event(e);
    }

    void update_key_modifiers(KeyboardEvent::Type event_type, KeyCode code);
    void update_mouse_position(int x, int y);

private:
    void begin_frame();
    void begin_imgui_frame();
    void end_imgui_frame();
    void end_frame();

protected:
    AbstractRenderModule* m_render_module{nullptr};
    KeyModifiers m_key_modifiers{KeyModifiers(KeyModifier::None)};
    int m_mouse_x;
    int m_mouse_y;
private:
    double m_last_time{0};
};

}
