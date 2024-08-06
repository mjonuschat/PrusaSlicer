#pragma once

#define USE_IMGUI_RENDER 1

#include <vector>

#include "AbstractRenderModule.hpp"
#include "IRenderRequestHandler.hpp"
#include "StdMainThreadDispatcher.hpp"
#include "ScreenInfo.hpp"

#if USE_IMGUI_RENDER
#include <Slic3r/App/Render/ImguiRender.hpp>
#endif

namespace Slic3r::App::Render {
class Device;
}


namespace Slic3r::App::Platform {

/**
 * Abstract class for platform-specific render canvas.
 *
 * Key responsibilities:
 * - facilitate rendering of render module
 * - translate platform specific events and push them the render module
 */
class AbstractRenderCanvas : public IRenderRequestHandler, public IMainThreadDispatcher
{
public:
    ~AbstractRenderCanvas() override = default;

    virtual void render();
    void set_render_module(AbstractRenderModule* render_module);

    // IRenderRequestHandler interface impl
    void request_render() override;

    void dispatch_on_main_thread(Function func) override
    { m_main_thread_dispatcher.dispatch_on_main_thread(func); }

    void dispatch_on_main_thread_after(Function func) override
    { m_main_thread_dispatcher.dispatch_on_main_thread_after(func); }

    bool dispatch_enqueued() override
    { return m_main_thread_dispatcher.dispatch_enqueued(); }

protected:
    virtual void begin_frame_platform() = 0;
    virtual void begin_imgui_frame_platform() = 0;
    virtual void end_imgui_frame_platform() = 0;
    virtual void end_frame_platform() = 0;
    virtual double get_platform_time() = 0;
    virtual Render::Device& get_device() = 0;

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
    void set_screen_size(const ScreenInfo& screen_info);

protected:
    using MouseEvents = std::vector<MouseEvent>;
    using KeyboardEvents = std::vector<KeyboardEvent>;

    AbstractRenderModule* m_render_module{nullptr};
    KeyModifiers m_key_modifiers{KeyModifiers(KeyModifier::None)};

    int m_mouse_x {0};
    int m_mouse_y {0};

    ScreenInfo m_screen_info{0,0,1};

    MouseEvents m_enqueued_mouse_events;
    KeyboardEvents m_enqueued_keyboard_events;
    StdMainThreadDispatcher m_main_thread_dispatcher;
private:
#if USE_IMGUI_RENDER
    std::unique_ptr<Render::ImguiRender> m_imgui_render;
#endif
    double m_last_time{0};
    size_t m_render_request_count{0};
};

}
