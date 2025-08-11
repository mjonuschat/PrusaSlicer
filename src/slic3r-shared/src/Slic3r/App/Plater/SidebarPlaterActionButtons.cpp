#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

using Biz::Scene::BedSelection;
using Biz::Scene::get_selected_beds;
using Biz::Scene::BedInstances;
using Domain::SlicingId;
using Biz::Slicing::Status;
using Domain::SelectionId;

SidebarPlaterActionButtons::SidebarPlaterActionButtons(Navigator* render_module_navigator) :
    SidebarActionButtons("sidebar_plater_action_buttons", Render::ModuleType::Plater, render_module_navigator)
{
    m_button_slice = emplace_back<LayoutButton>("Slice");
    m_button_slice->set_flex_grow(1);
    m_button_slice->set_background_color(color_primary);
    m_button_slice->set_min_size({0, button_height});
    m_button_slice->set_label_font_type(Render::ImguiFontType::Bold);
    m_button_slice->set_enabled(false);
}

SidebarPlaterActionButtons::~SidebarPlaterActionButtons()
{
    if (m_project_interactor != nullptr) {
        m_project_interactor->status_cache().remove_listener<Biz::IStatusCacheChangedListener>(this);
        m_project_interactor->scene_interactor()
            .remove_listener<Biz::ISelectedBedInstancesChangedListener>(this);
    }
}

void SidebarPlaterActionButtons::on_init(Biz::ProjectInteractor* project_interactor)
{
    m_project_interactor = project_interactor;
    m_project_interactor->status_cache().add_listener<Biz::IStatusCacheChangedListener>(this);
    m_project_interactor->scene_interactor().add_listener<Biz::ISelectedBedInstancesChangedListener>(
        this
    );
}

void SidebarPlaterActionButtons::on_status_cache_changed(const Domain::SlicingId slicing_id)
{
    const BedSelection& selection{m_project_interactor->scene_interactor().bed_selection()};
    update_slice_button(selection);
}

void SidebarPlaterActionButtons::on_selected_bed_instances_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::BedSelection& bed_selection
)
{
    update_slice_button(bed_selection);
}

struct BedStatus {
    SlicingId slicing_id;
    std::size_t bed_index;
    Status status;
};

void SidebarPlaterActionButtons::update_slice_button(const BedSelection& selection)
{
    if (m_project_interactor == nullptr) {
        return;
    }

    const Domain::SelectionId project_id{m_project_interactor->selected_project_id()};
    const BedInstances instances{get_selected_beds(
        project_id,
        selection,
        m_project_interactor->workbench()
    )};

    std::vector<BedStatus> statuses;
    for (const auto& bed_instance_ref : instances) {
        SlicingId slicing_id{project_id, bed_instance_ref.get().id().id};
        const auto status{m_project_interactor->status_cache().get_status(slicing_id)};
        if (status) {
            statuses.push_back({slicing_id, bed_instance_ref.get().index(), *status});
        } else {
            statuses.push_back({slicing_id, bed_instance_ref.get().index(), Status::InvalidData});
        }
    }

    if (statuses.empty()) {
        return;
    }

    const bool any_invalid{std::ranges::any_of(statuses, [](const auto& bed_status) {
        return bed_status.status == Status::InvalidData;
    })};

    const bool any_modified{std::ranges::any_of(statuses, [](const auto& bed_status) {
        return bed_status.status == Status::Modified;
    })};

    const bool previewable{std::ranges::any_of(statuses, [&](const auto& bed_status) {
        return bed_status.status == Status::Finished
            && bed_status.slicing_id.bed_instance_id == selection.last_selected_bed().instance_id;
    })};

    const bool any_running{std::ranges::any_of(statuses, [](const auto& bed_status) {
        return bed_status.status == Status::Running;
    })};


    m_button_slice->set_background_color(color_primary);
    m_button_slice->callbacks().action = []() {
    };
    m_button_slice->set_enabled(true);

    if (any_invalid) {
        m_button_slice->set_label("Invalid settings");
        m_button_slice->set_enabled(false);
    } else if (any_modified) {
        m_button_slice->set_label("Slice");
        m_button_slice->callbacks().action = [this, statuses]() {
            for (const BedStatus& bed_status : statuses) {
                if (bed_status.status == Status::Modified) {
                    m_project_interactor->slicing_interactor().slice_bed(bed_status.slicing_id);
                }
            }
            navigate_to_other();
        };
    } else if (previewable) {
        m_button_slice->set_label("Preview");
        m_button_slice->callbacks().action = [this]() {
            navigate_to_other();
        };
        m_button_slice->set_background_color(color_secondary);
    } else if (any_running) {
        m_button_slice->set_label("Cancel");
        m_button_slice->callbacks().action = [this, statuses]() {
            for (const BedStatus& bed_status : statuses) {
                if (bed_status.status == Status::Running) {
                    m_project_interactor->slicing_interactor().stop_slicing_bed(bed_status.slicing_id);
                }
            }
            navigate_to_other();
        };
    } else {
        m_button_slice->set_label("Slice");
        m_button_slice->set_enabled(false);
    }
}

} // namespace Slic3r::App::Plater
