#pragma once

#include <vector>
#include <libassert/assert.hpp>

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/Biz/Platform/IRenderRequestHandler.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"

#include <Slic3r/App/Render/ImguiRender.hpp>
#include <optional>

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
class AbstractRenderCanvas : public Biz::Platform::IRenderRequestHandler
{
public:
    AbstractRenderCanvas()
        : m_main_thread_dispatcher{Biz::Platform::PlatformServices::instance().main_thread_dispatcher()}
    {}

    ~AbstractRenderCanvas() override = default;

    const std::string& language() const { return m_imgui_render->language(); }
    void set_language(const std::string& language) { m_pending_language = language; }
    float font_size() const { return m_imgui_render->font_size(); }
    void set_font_size(float font_size) { m_pending_font_size = font_size; }

    virtual void render();
    void set_render_module(AbstractRenderModule* render_module);

    // IRenderRequestHandler interface impl
    void request_render() override;

protected:
    virtual void begin_frame_platform() = 0;
    virtual void begin_imgui_frame_platform() = 0;
    virtual void end_imgui_frame_platform() = 0;
    virtual void end_frame_platform() = 0;
    virtual double platform_time() = 0;
    virtual Render::Device& device() = 0;

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
    void set_screen_size(const Render::ScreenInfo& screen_info);

protected:
    using MouseEvents = std::vector<MouseEvent>;
    using KeyboardEvents = std::vector<KeyboardEvent>;

    AbstractRenderModule* m_render_module{nullptr};
    KeyModifiers m_key_modifiers{KeyModifiers(KeyModifier::None)};

    int m_mouse_x {0};
    int m_mouse_y {0};

    Render::ScreenInfo m_screen_info{0,0,1};

    MouseEvents m_enqueued_mouse_events;
    KeyboardEvents m_enqueued_keyboard_events;
    Biz::Platform::IMainThreadDispatcher& m_main_thread_dispatcher;
private:
    std::unique_ptr<Render::ImguiRender> m_imgui_render;
    std::optional<std::string> m_pending_language;
    std::optional<float> m_pending_font_size;
    double m_last_time{0};
    size_t m_render_request_count{0};
};

}
