#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/Biz/Arrange/Settings.hpp"
#include "Slic3r/App/Plater/ArrangeDialog.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"

namespace Slic3r::App::Scene {
class GeometryDataFactory;
} // namespace Slic3r::App::Scene

namespace Slic3r::Domain {
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {
class ProjectInteractor;
class ArrangeInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Plater {

class ArrangeGizmo final : public Scene::IToolGizmo, public Biz::ISelectedBedInstancesChangedListener
{
public:
    ArrangeGizmo(
        Biz::ArrangeInteractor& arrange_interactor,
        Render::Device& device,
        Scene::ISceneProvider& scene_provider,
        Scene::GeometryDataFactory& data_factory,
        Biz::ProjectInteractor& project_interactor,
        const Domain::Workbench& workbench
    );

    ~ArrangeGizmo();

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

    void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::BedSelection& bed_selection
    ) override;

    void on_activated() override;
    void on_deactivated() override;

    Scene::ToolType type() const override;

    Yoga::Dialog* unload_ui_dialog() override;

private:
    Biz::ArrangeInteractor& m_arrange_interactor;
    Render::Device& m_device;
    Scene::ISceneProvider& m_scene_provider;
    Scene::GeometryDataFactory& m_data_factory;
    Biz::ProjectInteractor& m_project_interactor;
    const Domain::Workbench& m_workbench;
    ArrangeDialog m_dialog;

    Biz::Arrange::Settings default_settings() const;
};
} // namespace Slic3r::App::Plater
