#pragma once

#include <vector>
#include <memory>

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"


namespace Slic3r::App::Plater {

class GizmoManager {
public:
    explicit GizmoManager(ISceneProvider& scene_provider) : m_scene_provider(scene_provider) {}
    void on_scene_mouse_event(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info);

    template<typename G, typename... ArgsT>
    G& add_base_gizmo(ArgsT&&... args)
    {
        m_base_gizmos.emplace_back(std::make_unique<G>(args...));
        auto& ptr = m_base_gizmos.back();
        return *static_cast<G*>(ptr.get());
    }

    void render_scene(Render::CommandBuffer& cmd_buffer);
    void render_imgui();

private:
    void prepare_cycle();

private:
    using IGizmoPtr = std::unique_ptr<IGizmo>;
    using IToolGizmoPtr = std::unique_ptr<IToolGizmo>;

    using GizmoList = std::vector<IGizmoPtr>;

    ISceneProvider& m_scene_provider;

    GizmoList m_base_gizmos;
    IToolGizmo* m_active_tool{nullptr};

    bool m_in_cycle {false};
    std::vector<IGizmo*> m_in_cycle_gizmos;
};

}

