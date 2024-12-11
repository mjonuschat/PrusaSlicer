#pragma once

#include <vector>
#include <memory>

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"
#include "Slic3r/App/Plater/GizmoDataFactory.hpp"
#include "libslic3r/Config.hpp"

namespace Slic3r::App::Plater {

class GizmoManager {
public:
    explicit GizmoManager(Render::Device& device, ISceneProvider& scene_provider)
        : m_scene_provider(scene_provider), m_data_factory(device)
    {}
    void on_scene_mouse_event(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info);

    template<typename G, typename... ArgsT>
    G& add_base_gizmo(ArgsT&&... args)
    {
        m_base_gizmos.emplace_back(std::make_unique<G>(args...));
        auto& ptr = m_base_gizmos.back();
        return *static_cast<G*>(ptr.get());
    }

    template<typename G, typename... ArgsT>
    G& add_tool_gizmo(ArgsT&&... args)
    {
        m_tool_gizmos.emplace_back(std::make_unique<G>(args...));
        auto& ptr = m_tool_gizmos.back();
        return *static_cast<G*>(ptr.get());
    }

    void render_scene(Render::CommandBuffer& cmd_buffer);
    void render_imgui();

    void toggle_activate_tool(ToolType tool, PrinterTechnology pt);
    void activate_tool(ToolType tool, PrinterTechnology pt);
    void deactivate_current_tool();

    GizmoDataFactory& data_factory() { return m_data_factory; }

private:
    void prepare_cycle();
    IToolGizmo* find_tool(ToolType tool, PrinterTechnology pt);

private:
    using IGizmoPtr = std::unique_ptr<IGizmo>;
    using IToolGizmoPtr = std::unique_ptr<IToolGizmo>;

    using GizmoList = std::vector<IGizmoPtr>;
    using ToolGizmoList = std::vector<IToolGizmoPtr>;

    ISceneProvider& m_scene_provider;

    GizmoDataFactory m_data_factory;

    GizmoList m_base_gizmos;
    ToolGizmoList m_tool_gizmos;
    IToolGizmo* m_active_tool{nullptr};

    bool m_in_cycle {false};
    std::vector<IGizmo*> m_in_cycle_gizmos;
};

}

