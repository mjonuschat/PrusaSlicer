#pragma once

#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Scene/Plane.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/MouseDragDetector.hpp"
#include "Slic3r/App/Plater/SelectionHandler.hpp"

namespace Slic3r::App::Plater {

using MousePosition = std::array<int, 2>;

class QuickDragGizmo : public Scene::IGizmo
{
public:
    QuickDragGizmo(
        Biz::Scene::SceneInteractor& scene_interactor,
        Scene::ISceneProvider& scene_provider,
        const Scene::MouseDragDetector& drag_detector
    );

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;

private:
    bool mouse_pos(float screen_x, float screen_y, Domain::Vec3d& out_pos);

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Scene::ISceneProvider& m_scene_provider;
    SelectionHandler m_selection_handler;
    const Scene::MouseDragDetector& m_drag_detector;
    Domain::Vec3d m_initial_world_pos;
    bool m_is_dragging = false; // additional condition for drag
    Scene::Plane m_plane{Domain::Vec3d::UnitZ(), 0};
    Biz::Scene::TransformMemento m_xform_memento;
};

} // namespace Slic3r::App::Plater
