#pragma once

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"

namespace Slic3r::App::Plater {

class GizmoDataFactory;

class RotationGizmo : public IToolGizmo
{
public:
    RotationGizmo(Render::Device& device, GizmoDataFactory& data_factory,
        ScenePresenter& scene_provider, Biz::Scene::SceneInteractor& scene_interactor);

    /**
     * @name Implementation of IGizmo interface
     * @{
     */
    GizmoActivationState on_mouse(GizmoEventContext& ctx, bool only_active) override;
    void on_transient_mouse(GizmoEventContext& ctx) override;
    void on_cycle_prepare() override;
    /**@}*/

    /**
     * @name Implementation of IToolGizmo interface
     * @{
     */
    void on_activated() override;
    void on_deactivated() override;
    ToolType type() const override { return ToolType::Rotation; }
    /**@}*/

private:
    void clear_highlight();
    void on_stop_dragging();

private:
    Render::Device& m_device;
    GizmoDataFactory& m_data_factory;
    ScenePresenter& m_scene_presenter;
    Biz::Scene::SceneInteractor& m_scene_interactor;
    bool m_activated{ false };
    bool m_dragging{ false };
    bool m_highlighted{ false };
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
    Vec3d m_pivot{ Vec3d::Zero()};
    Biz::Scene::TransformMemento m_xform_memento;
    Scene::Node::NodeList m_handles;
};

} // namespace Slic3r::App::Plater
