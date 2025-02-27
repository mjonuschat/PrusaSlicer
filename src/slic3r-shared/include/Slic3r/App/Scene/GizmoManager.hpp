#pragma once

#include <vector>
#include <memory>

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"
#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include "Slic3r/App/CommandRegistry.hpp"
#include "libslic3r/Config.hpp"


#ifndef DEBUG_GIZMO_MANAGER
#define DEBUG_GIZMO_MANAGER 0
#endif


namespace Slic3r::App::Scene {

class GizmoManager {
public:
    explicit GizmoManager(Render::Device& device, ISceneProvider& scene_provider)
        : m_scene_provider(scene_provider), m_data_factory(device)
    {}
    void on_scene_mouse_event(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info);
    bool on_scene_keyboard_event(const Platform::KeyboardEvent& e);

    template<typename G, typename... ArgsT>
    G& add_base_gizmo(ArgsT&&... args)
    {
        m_base_gizmos.emplace_back(std::make_unique<G>(args...));
        auto& ptr = m_base_gizmos.back();
        ptr->register_commands(m_command_registry);
        return *static_cast<G*>(ptr.get());
    }

    template<typename G, typename... ArgsT>
    G& add_tool_gizmo(ArgsT&&... args)
    {
        m_tool_gizmos.emplace_back(std::make_unique<G>(args...));
        auto& ptr = m_tool_gizmos.back();
        ptr->register_commands(m_command_registry);
        return *static_cast<G*>(ptr.get());
    }

    void render_scene(Render::CommandBuffer& cmd_buffer);
    void render_imgui();

    void toggle_activate_tool(ToolType tool, PrinterTechnology pt);
    void activate_tool(ToolType tool, PrinterTechnology pt);
    void deactivate_current_tool();
    ToolType current_tool_type() const { return (m_active_tool != nullptr) ? m_active_tool->type() : ToolType::None; }

    GeometryDataFactory& data_factory() { return m_data_factory; }

private:
    void prepare_cycle();
    IToolGizmo* find_tool(ToolType tool, PrinterTechnology pt);

#if DEBUG_GIZMO_MANAGER
    void update_gizmo_activation_debug_data(const IGizmo* g, GizmoActivationState state);
    void update_gizmo_activation_debug_frame_begin();
    void render_gizmo_activation_debug();
#endif

private:
    using IGizmoPtr = std::unique_ptr<IGizmo>;
    using IToolGizmoPtr = std::unique_ptr<IToolGizmo>;

    using GizmoList = std::vector<IGizmoPtr>;
    using ToolGizmoList = std::vector<IToolGizmoPtr>;

    ISceneProvider& m_scene_provider;

    GeometryDataFactory m_data_factory;

    GizmoList m_base_gizmos;
    ToolGizmoList m_tool_gizmos;
    IToolGizmo* m_active_tool{nullptr};

    bool m_in_cycle {false};
    std::vector<IGizmo*> m_in_cycle_gizmos;

    CommandRegistry m_command_registry;

#if DEBUG_GIZMO_MANAGER
    constexpr static size_t NUM_DEBUG_ACTIVATION_LAST_STEPS = 63;
    using GizmoActivationDebugData = std::list<GizmoActivationState>;
    using GizmosActivationDebugData = std::unordered_map<const IGizmo*, GizmoActivationDebugData>;
    bool m_activation_debug_shown {true};
    GizmosActivationDebugData m_activation_debug;
#endif
};

} // namespace Slic3r::App::Scene

