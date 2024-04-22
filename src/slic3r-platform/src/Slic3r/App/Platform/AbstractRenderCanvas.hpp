#pragma once

#include <vector>

#include "AbstractRenderModule.hpp"
#include "IRenderRequestHandler.hpp"

namespace Slic3r::App::Platform {

/**
 * Abstract class for platform-specific render canvas.
 *
 * Key responsibilities:
 * - facilitate rendering of render module
 * - translate platform specific events and push them the render module
 */
class AbstractRenderCanvas : public IRenderRequestHandler
{
public:
    ~AbstractRenderCanvas() override = default;

    void render();
    void set_render_module(AbstractRenderModule* render_module);

    // IRenderRequestHandler interface impl
    void request_render() override;

protected:
    virtual void begin_frame_platform() = 0;
    virtual void begin_imgui_frame_platform() = 0;
    virtual void end_imgui_frame_platform() = 0;
    virtual void end_frame_platform() = 0;
    virtual double get_platform_time() = 0;

    void enqueue_mouse(const MouseEvent& e);
    void enqueue_keyboard(const KeyboardEvent& e);

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

    bool get_and_reset_render_requested();

private:
    void begin_frame();
    void begin_imgui_frame();
    void end_imgui_frame();
    void end_frame();
    void emit_enqueued_events();

protected:
    using MouseEvents = std::vector<MouseEvent>;
    using KeyboardEvents = std::vector<KeyboardEvent>;

    AbstractRenderModule* m_render_module{nullptr};
    KeyModifiers m_key_modifiers{KeyModifiers(KeyModifier::None)};

    int m_mouse_x;
    int m_mouse_y;

    MouseEvents m_enqueued_mouse_events;
    KeyboardEvents m_enqueued_keyboard_events;
private:
    double m_last_time{0};
    bool m_render_requested{false};
};

}
