#include "Slic3r/App/Plater/ArrangeGizmo.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Biz/ArrangeInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/Workbench.hpp"

namespace Slic3r::App::Plater {

using Biz::Algorithms::Scaling::scaled;
using Biz::Arrange::Settings;
using Biz::Scene::BedSelection;
using Domain::Bed;
using Domain::BedRefs;
using Domain::coord_t;
using Domain::Project;
using Domain::SelectionId;
using Domain::Vec2f;
using Yoga::ItemPtr;
using NodeList = Scene::Node::NodeList;
using Biz::Arrange::Mode;
using Biz::Platform::PlatformServices;
using Biz::Platform::JobManager::JobManagerStatus;
using Biz::Platform::JobManager::Progress;
using Biz::Scene::BedSelectionMode;
using Biz::Scene::SceneInteractor;
using Domain::ConfigContainer;
using Domain::JobStatus;

namespace {
void
update_settings_from_bed(const Project& project, const BedSelection& selection, Settings& settings)
{
    const ConfigContainer* config_container{
        project.find_config_container(selection.config_container_id())
    };
    if (!config_container) {
        return;
    }
    const Bed& bed{config_container->bed()};
    settings.bed_segments = bed.segments();
    settings.auxiliary_travel_anchor = bed.auxiliary_travel_anchor();
}
} // namespace

ArrangeGizmo::ArrangeGizmo(
    Biz::ArrangeInteractor& arrange_interactor,
    Render::Device& device,
    Scene::ISceneProvider& scene_provider,
    Scene::GeometryDataFactory& data_factory,
    Biz::ProjectInteractor& project_interactor,
    const Domain::Workbench& workbench
) :
    m_arrange_interactor{arrange_interactor},
    m_device{device},
    m_scene_provider{scene_provider},
    m_data_factory{data_factory},
    m_project_interactor{project_interactor},
    m_workbench{workbench},
    m_dialog(
        std::make_unique<ArrangeDialog>(
            [this](const Settings& settings)
            { m_arrange_interactor.arrange(m_project_interactor.selected_project_id(), settings); },
            []() { PlatformServices::instance().job_manager().cancel_job("arrange"); },
            [this](const Mode mode)
            {
                Biz::Platform::JobManager::JobManager& job_manager{PlatformServices::instance().job_manager()};
                job_manager.cancel_job("arrange");
                SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
                BedSelection& selection{scene_interactor.bed_selection()};
                if (mode == Mode::Local) {
                    selection.set_mode(BedSelectionMode::SingleBed);
                } else if (mode == Mode::Global) {
                    selection.set_mode(BedSelectionMode::ConfigContainer);
                }
            },
            default_settings()
        )
    )
{
    m_project_interactor.scene_interactor().add_listener<Biz::ISelectedBedInstancesChangedListener>(this);

    PlatformServices::instance().job_manager().add_listener<Biz::Platform::JobManager::IJobManagerStatusChangedListener>(this);
}

ArrangeGizmo::~ArrangeGizmo()
{
    m_project_interactor.scene_interactor().remove_listener<Biz::ISelectedBedInstancesChangedListener>(this);
    PlatformServices::instance().job_manager().remove_listener<Biz::Platform::JobManager::IJobManagerStatusChangedListener>(this);
}

Scene::GizmoActivationState ArrangeGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    return Scene::GizmoActivationState::Inactive;
};

void ArrangeGizmo::on_selected_bed_instances_changed(SelectionId project_id, const BedSelection& bed_selection)
{
    const Project& project{m_workbench.project(project_id)};
    Settings settings;
    update_settings_from_bed(project, bed_selection, settings);
    m_dialog->set_bed_segments(settings.bed_segments);
    m_dialog->set_auxiliary_travel_anchor(settings.auxiliary_travel_anchor);
};

void ArrangeGizmo::on_job_manager_status_changed(const Biz::Platform::JobManager::JobManagerStatus& status)
{
    const auto it{status.find("arrange")};
    if (it == status.end()) {
        m_dialog->update_status(ArrangeTaskStatus::Idle);
        return;
    }

    const Progress progress{it->second};
    if (progress.status == JobStatus::Started) {
        m_dialog->update_status(ArrangeTaskStatus::Running);
        return;
    }
    m_dialog->update_status(ArrangeTaskStatus::Idle);
}

void ArrangeGizmo::on_activated()
{
    m_active = true;
    if (m_dialog->get_arrange_mode() == Mode::Global) {
        SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
        scene_interactor.bed_selection().set_mode(BedSelectionMode::ConfigContainer);
    }
};

void ArrangeGizmo::on_deactivated()
{
    m_active = false;
    SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
    scene_interactor.bed_selection().set_mode(BedSelectionMode::SingleBed);

    Biz::Platform::JobManager::JobManager& job_manager{PlatformServices::instance().job_manager()};
    job_manager.cancel_job("arrange");
};

Scene::ToolType ArrangeGizmo::type() const
{
    return Scene::ToolType::ArrangeGizmo;
}

bool ArrangeGizmo::enabled() const
{
    return true;
}

Yoga::GizmoWindowPtr ArrangeGizmo::release_ui_window()
{
    return m_dialog.release();
}

Settings ArrangeGizmo::default_settings() const
{
    Settings settings;
    settings.scaled_offset = scaled(3.0);

    const Project& project{m_project_interactor.selected_project()};
    const BedSelection& bed_selection{m_project_interactor.scene_interactor().bed_selection()};
    update_settings_from_bed(project, bed_selection, settings);
    return settings;
}

} // namespace Slic3r::App::Plater
