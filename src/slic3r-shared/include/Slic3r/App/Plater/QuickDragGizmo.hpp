#pragma once

#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Scene/Plane.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/MouseDragDetector.hpp" // IMouseDragCallbacks
#include "Slic3r/App/Plater/SelectionHandler.hpp"

namespace Slic3r::App::Plater {

using MousePosition = std::array<int, 2>;

class QuickDragGizmo : public Scene::IGizmo
{
public:
    QuickDragGizmo(
        Biz::Scene::SceneInteractor& scene_interactor,
        Scene::ISceneProvider& scene_provider,
        Scene::IMouseDragCallbacks& drag_callbacks
    );

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
private:
    bool mouse_pos(float screen_x, float screen_y, Domain::Vec3d& out_pos);

    bool on_drag_start(const Scene::GizmoEventContext& ctx);
    bool on_draging(const Scene::GizmoEventContext& ctx);
    void on_drag_finish();
    void on_drag_cancel();

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Scene::ISceneProvider& m_scene_provider;
    SelectionHandler m_selection_handler;
    Domain::Vec3d m_initial_world_pos;
    Scene::Plane m_plane{Domain::Vec3d::UnitZ(), 0};
    Biz::Scene::TransformMemento m_xform_memento;
};

} // namespace Slic3r::App::Plater
