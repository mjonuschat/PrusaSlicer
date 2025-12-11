#pragma once

#include "Slic3r/App/Platform/MouseEvent.hpp"
#include "Slic3r/App/Platform/KeyboardEvent.hpp"
#include "Slic3r/Biz/Platform/IRenderRequestHandler.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"
#include "Slic3r/App/Platform/CommandRegistry.hpp"
#include "Slic3r/App/Platform/CameraSynchData.hpp"

namespace Slic3r::App {
class Navigator;
} // namespace Slic3r::App

namespace Slic3r::App::Platform {
class AnimationManager;
} // namespace Slic3r::App::Platform

namespace Slic3r::App::Render {
class Device;
class CommandBuffer;
class ImguiRender;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Platform {

/**
 * Provides abstract interface for render module and common infrastructure for rendering
 * and event processing.
 */
class AbstractRenderModule
{
public:
    virtual ~AbstractRenderModule() = default;

    virtual void render_scene(Render::CommandBuffer& cmd_buffer) = 0;
    virtual void render_imgui(Render::CommandBuffer& cmd_buffer) = 0;

    virtual void on_scene_mouse_event(const MouseEvent& e);
    virtual void on_scene_keyboard_event(const KeyboardEvent& e);

    void activate(Biz::Platform::IRenderRequestHandler* render_request_handler);
    void deactivate();

    void set_screen_size(const Render::ScreenInfo& screen_info);
    void ensure_initialized(Render::Device& device, Render::ImguiRender& imgui_render, Platform::AnimationManager& animation_manager);

    virtual const std::optional<CameraSynchData>& camera_synch_data() const = 0;
    virtual void set_camera_synch_data(const CameraSynchData& data) = 0;

    void set_imgui_render(Render::ImguiRender* imgui_render);
    virtual void set_sidebars_visible(bool visible) {};

protected:
    /**
     * Initialize all Render objects here.
     */
    virtual void on_init(Render::Device& device, Render::ImguiRender& imgui_render, Platform::AnimationManager& animation_manager);

    virtual void on_activated();
    virtual void on_deactivated();
    virtual void on_screen_resized();
    virtual void on_set_imgui_render() {}

    virtual void register_commands() {}
    void request_render();

    virtual void set_navigator(Navigator* n) = 0;

protected:
    Render::Device* m_device{nullptr};
    CommandRegistry m_command_registry;
    Render::ImguiRender* m_imgui_render{nullptr};
    Platform::AnimationManager* m_animation_manager{nullptr};

    Render::ScreenInfo m_screen_info{0, 0, 1};
    bool m_initialized{false};
private:
    Biz::Platform::IRenderRequestHandler* m_render_request_handler{nullptr};
};

} // namespace Slic3r::App::Platform
