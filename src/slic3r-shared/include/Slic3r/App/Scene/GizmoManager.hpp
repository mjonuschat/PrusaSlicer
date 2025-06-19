#pragma once

#include <vector>
#include <memory>

#include <Slic3r/App/Platform/CommandRegistry.hpp>
#include <Slic3r/Biz/Platform/ListenerScope.hpp>

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"
#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include "libslic3r/Config.hpp"
#include "Slic3r/Biz/ProjectScoped.hpp"


#ifndef DEBUG_GIZMO_MANAGER
#define DEBUG_GIZMO_MANAGER 0
#endif


namespace Slic3r::App::Scene {

class IGizmoActiveToolListener {
public:
    virtual ~IGizmoActiveToolListener() = default;

    virtual void active_tool_changed(IToolGizmo* active_tool) = 0;
};

class GizmoManager : public WithListeners<IGizmoActiveToolListener>, public Biz::ISelectedProjectChangedListener, public Biz::IProjectsChangedListener {
public:
    GizmoManager(Render::Device& device, ISceneProvider& scene_provider, Biz::ProjectInteractor& project_interactor);

    void on_scene_mouse_event(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info);
    bool on_scene_keyboard_event(const Platform::KeyboardEvent& e);
    void on_project_will_be_removed(Domain::SelectionId project_id) override;

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

    void toggle_activate_tool(ToolType tool, PrinterTechnology pt);
    void activate_tool(ToolType tool, PrinterTechnology pt);
    void deactivate_current_tool();
    ToolType current_tool_type() const;

    GeometryDataFactory& data_factory() { return m_data_factory; }

    void render_imgui();

private:
    void on_selected_project_changed(size_t index) override;

    void prepare_cycle();
    IToolGizmo* find_tool(ToolType tool, PrinterTechnology pt);

    struct ProjectContext;
    ProjectContext& current_context() { return m_projects.selected(); }
    const ProjectContext& current_context() const { return m_projects.selected(); }
#if DEBUG_GIZMO_MANAGER
    void update_gizmo_activation_debug_data(const IGizmo* g, GizmoActivationState state);
    void update_gizmo_activation_debug_frame_begin();
    void render_gizmo_activation_debug();
#endif

private:
    Biz::ListenerScope<Biz::IProjectsChangedListener, Biz::ProjectInteractor, GizmoManager>
        m_project_changed_listener_scope;
    Biz::ListenerScope<Biz::ISelectedProjectChangedListener, Biz::ProjectInteractor, GizmoManager>
        m_selected_project_changed_listener_scope;

    using IGizmoPtr = std::unique_ptr<IGizmo>;
    using IToolGizmoPtr = std::unique_ptr<IToolGizmo>;

    using GizmoList = std::vector<IGizmoPtr>;
    using ToolGizmoList = std::vector<IToolGizmoPtr>;

    struct ProjectContext
    {
        IToolGizmo* active_tool{nullptr};

        bool in_cycle {false};
        std::vector<IGizmo*> in_cycle_gizmos;

    };
    using ProjectContexts = Biz::ProjectScoped<ProjectContext>;

    ProjectContexts m_projects;
    ISceneProvider& m_scene_provider;
    Biz::ProjectInteractor& m_project_interactor;

    GeometryDataFactory m_data_factory;
    Domain::SelectionId m_last_project_id{Domain::INVALID_ID};

    GizmoList m_base_gizmos;
    ToolGizmoList m_tool_gizmos;
    Platform::CommandRegistry m_command_registry;

#if DEBUG_GIZMO_MANAGER
    constexpr static size_t NUM_DEBUG_ACTIVATION_LAST_STEPS = 63;
    using GizmoActivationDebugData = std::list<GizmoActivationState>;
    using GizmosActivationDebugData = std::unordered_map<const IGizmo*, GizmoActivationDebugData>;
    bool m_activation_debug_shown {true};
    GizmosActivationDebugData m_activation_debug;
#endif
};

} // namespace Slic3r::App::Scene

