///|/ Copyright (c) Prusa Research 2025 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Scene/GizmoEventContext.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

namespace Slic3r::Biz::Emboss {

class SurfaceDrag {
public:
    SurfaceDrag(App::Plater::PlaterScenePresenter& scene_presenter, Biz::ProjectInteractor& project_interactor);
    ~SurfaceDrag(); // for pimpl
    bool on_drag_start(const App::Scene::GizmoEventContext& ctx);
    bool on_dragging(const App::Scene::GizmoEventContext& ctx, 
        const std::optional<float>& angle,
        const std::optional<float>& distance,
        const std::optional<double>& up_limit);
    void on_drag_finish();
    void on_drag_cancel();

    bool is_dragging();
    void imgui_draw();

private:
    App::Plater::PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    struct Drag; // pimpl
    std::unique_ptr<Drag> m_drag; // exist only during drag operation
};

Domain::Transform3d get_volume_transformation(
    Domain::Transform3d world, // from volume
    const Domain::Vec3d& world_dir, // wanted new direction
    const Domain::Vec3d& world_position, // wanted new position
    // Invers transformation of text volume instance
    // Help convert world transformation to instance space 
    const Domain::Transform3d& instance_inv,
    // initial rotation in Z axis
    const std::optional<float>& current_angle,
    const std::optional<float>& current_distance,
    const std::optional<double>& up_limit);

} // Slic3r::Biz::Emboss