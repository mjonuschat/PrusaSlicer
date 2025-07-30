#include <span>

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
using Domain::BedRef;
using Domain::BedRefs;
using Domain::coord_t;
using Domain::Project;
using Domain::SelectionId;
using Domain::Vec2f;
using Yoga::ItemPtr;
using NodeList = Scene::Node::NodeList;

namespace {
std::optional<Bed::Segments> get_bed_segments(const Project& project, const BedRefs& bed_refs)
{
    if (bed_refs.empty()) {
        return std::nullopt;
    }

    const Bed& first_bed{project.find_config_container(bed_refs.front().config_container_id)->bed()};

    const std::optional<Bed::Segments> first_bed_segments{first_bed.segments()};
    if (!first_bed_segments) {
        return std::nullopt;
    }

    for (const BedRef& bed_ref : std::span{bed_refs}.subspan(1)) {
        const Bed& bed{project.find_config_container(bed_ref.config_container_id)->bed()};
        if (bed.segments() != first_bed_segments) {
            return std::nullopt;
        }
    }

    return first_bed_segments;
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
    m_dialog{
        [this](const Settings settings) { m_arrange_interactor.arrange(settings); },
        default_settings()
    }
{
    m_project_interactor.scene_interactor().add_listener<Biz::ISelectedBedInstancesChangedListener>(
        this
    );
}

ArrangeGizmo::~ArrangeGizmo()
{
    m_project_interactor.scene_interactor()
        .remove_listener<Biz::ISelectedBedInstancesChangedListener>(this);
}

Scene::GizmoActivationState ArrangeGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    return Scene::GizmoActivationState::Inactive;
};

void ArrangeGizmo::on_selected_bed_instances_changed(SelectionId project_id, const BedSelection& bed_selection)
{
    const Project& project{m_workbench.project(project_id)};
    const std::optional<Bed::Segments> bed_segments{get_bed_segments(project, bed_selection.all())};
    m_dialog.set_bed_segments(bed_segments);
};

void ArrangeGizmo::on_activated() {};

void ArrangeGizmo::on_deactivated() {};

Scene::ToolType ArrangeGizmo::type() const
{
    return Scene::ToolType::ArrangeGizmo;
}

Yoga::Dialog* ArrangeGizmo::unload_ui_dialog()
{
    return &m_dialog;
}

Settings ArrangeGizmo::default_settings() const
{
    Settings settings;
    settings.scaled_offset = scaled(3.0);

    const Project& project{m_project_interactor.selected_project()};
    const BedSelection& bed_selection{m_project_interactor.scene_interactor().bed_selection()};
    const std::optional<Bed::Segments> bed_segments{get_bed_segments(project, bed_selection.all())};

    settings.bed_segments = bed_segments;
    return settings;
}

} // namespace Slic3r::App::Plater
