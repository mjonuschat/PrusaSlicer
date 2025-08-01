///|/ Copyright (c) Prusa Research 2025 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Scene/Plane.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/MouseDragDetector.hpp" // IMouseDrag
#include "Slic3r/App/Plater/SelectionHandler.hpp"

namespace Slic3r::App::Plater {

using MousePosition = std::array<int, 2>;

class QuickDragGizmo : public Scene::IGizmo, public Scene::IMouseDrag
{
public:
    QuickDragGizmo(Biz::Scene::SceneInteractor& scene_interactor, Scene::ISceneProvider& scene_provider);

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

    // IMouseDrag
    bool on_drag_start(const Scene::GizmoEventContext& ctx) override;
    bool on_dragging(const Scene::GizmoEventContext& ctx) override;
    void on_drag_finish() override;
    void on_drag_cancel() override;

private:
    bool mouse_pos(float screen_x, float screen_y, Domain::Vec3d& out_pos);

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Scene::ISceneProvider& m_scene_provider;
    SelectionHandler m_selection_handler;
    Domain::Vec3d m_initial_world_pos;
    Scene::Plane m_plane{Domain::Vec3d::UnitZ(), 0};
    Biz::Scene::TransformMemento m_xform_memento;
};

} // namespace Slic3r::App::Plater
