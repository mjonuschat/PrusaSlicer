#pragma once

#include "PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Plater/ScaleDialog.hpp"

namespace Slic3r::App::Scene {
class GeometryDataFactory;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {

class ScaleGizmo :
    public Scene::IToolGizmo,
    public Biz::Scene::ISceneSelectionChangedListener,
    App::Plater::ISelectionBoundingBoxChangedListener
{
public:
    ScaleGizmo(
        Render::Device& device,
        Scene::GeometryDataFactory& data_factory,
        PlaterScenePresenter& scene_provider,
        Biz::ProjectInteractor& project_interactor
    );
    ~ScaleGizmo();

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void on_transient_mouse(Scene::GizmoEventContext& ctx) override;
    void on_cycle_prepare() override;
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override { return Scene::ToolType::Scale; }

    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;

    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection&) override;
    void on_scene_selection_transformed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection&) override;
    void on_scene_selection_bounding_box_changed(
        Domain::SelectionId project_id,
        const std::optional<Scene::OrientedBoundingBox>&
    ) override;

private:
    void update_handle_nodes();
    std::unique_ptr<Scene::Node> generate_handle_nodes() const;
    Render::Device& m_device;
    Scene::GeometryDataFactory& m_data_factory;
    PlaterScenePresenter& m_scene_provider;
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Biz::ProjectInteractor& m_project_interactor;
    Scene::Node* m_handle_nodes{nullptr};
    bool m_activated{false};

    Biz::Scene::TransformMemento m_xform_memento;
    Scene::Ray m_scale_ray;
    AxisType m_scale_axis{AxisType::None};
    Scene::OrientedBoundingBox m_start_obb;
    bool m_was_floating{false};
    double m_start_t{0};
    bool m_dragging{false};
    ScaleDialog* m_window;
};
}
