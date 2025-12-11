#pragma once
#include <functional>

#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp" // IToolGizmo
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/ProjectScoped.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener + ISceneChangedListener
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Plater {
class SimplifyDialog;

// Continue development for GLGizmoSimplify permanent link: 
// https://github.com/prusa3d/PrusaSlicer/blob/6fd9846df131c671ac9f944c836536f04d354a53/src/slic3r/GUI/Gizmos/GLGizmoSimplify.hpp
// https://github.com/prusa3d/PrusaSlicer/blob/6fd9846df131c671ac9f944c836536f04d354a53/src/slic3r/GUI/Gizmos/GLGizmoSimplify.cpp
class SimplifyGizmo : 
    public Scene::IToolGizmo, 
    public Biz::Scene::ISceneSelectionChangedListener,
    public Biz::Scene::ISceneChangedListener
{
public:
    using CloseFn = std::function<void()>;
    SimplifyGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor& project_interactor,
        CloseFn close_fn
    );
    ~SimplifyGizmo() override;
    /**
     * @name Implementation of IGizmo interface
     * @{
     */
    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    /**@}*/

    /**
     * @name Implementation of IToolGizmo interface
     * @{
     */
    void on_activated() override;
    void on_deactivated() override;
    void on_project_activated(size_t new_project_id) override;
    void on_project_deactivated(size_t old_project_id) override;
    Scene::ToolType type() const override;
    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;
    /**@}*/

    /**
     * @name Implementation of ISceneSelectionChangedListener interface
     * Change simplified selection
     */
    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;

    /**
     * @name Implementation of ISceneChangedListener interface
     * For external(not by SimplifyGizmo) transformation of the simplified mesh
     * Move simplified phantom to current position
     * @{
     */
    void on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements,
        Biz::Scene::TransformState state, const Biz::BedTrackingChanges& bed_tracking_changes) override;
    void on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements,
        Biz::Scene::TransformState state, const Biz::BedTrackingChanges& bed_tracking_changes) override;
    /**@}*/
private:
    SimplifyDialog& dialog() { return *m_dialog.get(); }
    void on_selection_change(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection);

    void deactivate(Domain::SelectionId project_id);
    void close();

    void apply_simplify();
    void process();
        
    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    CloseFn m_close_fn; // call GizmoManager to close current gizmo

    struct ProjectContext; // forward declaration
    // m_projects use pimpl to hide ProjectContext into cpp file
    std::unique_ptr<Biz::ProjectScoped<ProjectContext>> m_proj_ctxs;
    Yoga::Passthrough<SimplifyDialog> m_dialog;
};
} // namespace Slic3r::App::Plater
