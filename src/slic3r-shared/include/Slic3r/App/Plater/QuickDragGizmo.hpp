#pragma once

#include "libslic3r/Point.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Scene/Plane.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Plater/SelectionHandler.hpp"


namespace Slic3r::App::Plater {

using MousePosition = std::array<int, 2>;

class QuickDragGizmo : public Scene::IGizmo
{
public:
    QuickDragGizmo(Biz::Scene::SceneInteractor& scene_interactor, Scene::ISceneProvider& scene_provider)
        : m_scene_interactor(scene_interactor)
        , m_scene_provider(scene_provider)
        , m_selection_handler(scene_interactor)
    {}

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;
private:
    int mouse_dist_sq(int mouse_x, int mouse_y) const
    {
        const Vec2d initial_position{m_initial_mouse_pos[0], m_initial_mouse_pos[1]};
        const Vec2d mouse_position{mouse_x, mouse_y};

        return static_cast<int>((initial_position - mouse_position).squaredNorm());
    };
    bool mouse_pos(float screen_x, float screen_y, Vec3d& out_pos);

private:
    enum class State {
        Inactive = 0,
        Probing,
        Dragging
    };

    Biz::Scene::SceneInteractor& m_scene_interactor;
    Scene::ISceneProvider& m_scene_provider;
    SelectionHandler m_selection_handler;
    MousePosition m_initial_mouse_pos;
    Vec3d m_initial_world_pos;
    State m_state{State::Inactive};
    Scene::Plane m_plane{Vec3d::UnitZ(), 0};
    Biz::Scene::TransformMemento m_xform_memento;
    static constexpr int THRESHOLD_DIST_SQ = 5 * 5;
};

}
