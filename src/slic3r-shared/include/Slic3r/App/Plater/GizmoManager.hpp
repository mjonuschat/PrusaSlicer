#pragma once

#include <vector>
#include <memory>

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"


namespace Slic3r::App::Plater {

class GizmoManager {
public:
    explicit GizmoManager(Scene::Scene& scene) : m_scene(scene) {}
    void on_scene_mouse_event(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info);

    template<typename G, typename... ArgsT>
    G& add_base_gizmo(ArgsT&&... args)
    {
        m_base_gizmos.emplace_back(std::make_unique<G>(args...));
        auto& ptr = m_base_gizmos.back();
        return *static_cast<G*>(ptr.get());
    }

private:
    void prepare_cycle();

private:
    using IGizmoPtr = std::unique_ptr<IGizmo>;
    using IToolGizmoPtr = std::unique_ptr<IToolGizmo>;

    using GizmoList = std::vector<IGizmoPtr>;

    Scene::Scene& m_scene;

    GizmoList m_base_gizmos;
    IToolGizmo* m_active_tool{nullptr};

    bool m_in_cycle {false};
    std::vector<IGizmo*> m_in_cycle_gizmos;
};

}

