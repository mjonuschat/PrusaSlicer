#pragma once

#include "libslic3r/Point.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Scene/Plane.hpp"
#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Plater/SelectionHandler.hpp"


namespace Slic3r::App::Plater {

class QuickDragGizmo : public IGizmo {
public:
    QuickDragGizmo(Biz::Scene::SceneInteractor& scene_interactor, ISceneProvider& scene_provider)
        : m_scene_interactor(scene_interactor)
        , m_scene_provider(scene_provider)
        , m_selection_handler(scene_interactor)
    {}

    GizmoActivationState on_mouse(GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;
private:
    int mouse_dist_sq(int mouse_x, int mouse_y) const
    {
        return (m_initial_mouse_pos - Vec2i{mouse_x, mouse_y}).squaredNorm();
    };
    bool mouse_pos(float screen_x, float screen_y, Vec3d& out_pos);

private:
    enum class State {
        Inactive = 0,
        Probing,
        Dragging
    };

    Biz::Scene::SceneInteractor& m_scene_interactor;
    ISceneProvider& m_scene_provider;
    SelectionHandler m_selection_handler;
    Vec2i m_initial_mouse_pos;
    Vec3d m_initial_world_pos;
    State m_state{State::Inactive};
    Scene::Plane m_plane{Vec3d::UnitZ(), 0};
    Biz::Scene::TransformMemento m_xform_memento;
    static constexpr int THRESHOLD_DIST_SQ = 5 * 5;
};

}
