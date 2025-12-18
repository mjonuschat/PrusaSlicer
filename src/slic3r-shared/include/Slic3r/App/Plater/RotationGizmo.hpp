#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/App/Plater/RotationDialog.hpp"

namespace Slic3r::Biz {
    class ProjectInteractor;
}

namespace Slic3r::App::Scene {
class GeometryDataFactory;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {

class PlaterScenePresenter;

class RotationGizmo : public Scene::IToolGizmo, public ISelectionBoundingBoxChangedListener
{
public:
    RotationGizmo(Render::Device& device, Scene::GeometryDataFactory& data_factory,
        PlaterScenePresenter& scene_presenter, Biz::ProjectInteractor& project_interactor);

    ~RotationGizmo();

    /**
     * @name Implementation of IGizmo interface
     * @{
     */
    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void on_transient_mouse(Scene::GizmoEventContext& ctx) override;
    void on_cycle_prepare() override;
    /**@}*/

    /**
     * @name Implementation of IToolGizmo interface
     * @{
     */
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override { return Scene::ToolType::Rotation; }
    /**@}*/

    void on_scene_selection_bounding_box_changed(
        Domain::SelectionId project_id,
        const std::optional<Scene::OrientedBoundingBox>&
    ) override;

    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;

private:
    void on_stop_dragging();
    void add_highlight_node(AxisType axis);
    void remove_highlight_node();

private:
    Render::Device& m_device;
    Scene::GeometryDataFactory& m_data_factory;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Scene::SceneInteractor& m_scene_interactor;
    bool m_activated{ false };
    bool m_dragging{ false };
    AxisType m_curr_axis{ AxisType::None };
    Scene::Ray m_translation_ray;
    struct Snap
    {
        struct Radii
        {
            double in{ 0.0 };
            double out{ 0.0 };
        };
        Radii coarse;
        Radii fine;
    };
    Snap m_snap;
    Domain::Vec2d m_start_direction{Domain::Vec2d::Zero()};
    Scene::OrientedBoundingBox m_start_obb;
    bool m_was_floating{false};
    Biz::Scene::TransformMemento m_xform_memento;
    RotationDialog* m_window{nullptr};
    Scene::Node* m_highlight_node{nullptr};
    Scene::Node::NodeList m_handles;
};

} // namespace Slic3r::App::Plater
