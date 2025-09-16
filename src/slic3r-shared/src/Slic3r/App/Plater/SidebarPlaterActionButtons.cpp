#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"

#include "Slic3r/App/DisplayStrings.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

using Biz::Scene::BedInstances;
using Biz::Scene::BedSelection;
using Biz::Scene::get_selected_beds;
using Biz::Slicing::StatusCode;
using Domain::SelectionId;
using Domain::SlicingId;

SidebarPlaterActionButtons::SidebarPlaterActionButtons(Navigator* render_module_navigator) :
    SidebarActionButtons("sidebar_plater_action_buttons", Render::ModuleType::Plater, render_module_navigator)
{
    set_gap(5);
    auto navigation_button{get_navigation_button()};
    m_navigation_button = navigation_button.get();
    append(std::move(navigation_button));

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

void SidebarPlaterActionButtons::on_status_cache_status_code_changed(const Domain::SlicingId slicing_id)
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

struct BedStatus
{
    SlicingId slicing_id;
    std::size_t bed_index;
    StatusCode status;
    std::vector<std::string> errors;
    std::vector<std::string> warrnings;
};

void SidebarPlaterActionButtons::update_slice_button(const BedSelection& selection)
{
    if (m_project_interactor == nullptr) {
        return;
    }

    const Domain::SelectionId project_id{m_project_interactor->selected_project_id()};
    const Domain::Project& project{m_project_interactor->workbench().project(project_id)};
    const BedInstances instances{
        get_selected_beds(project_id, selection, m_project_interactor->workbench())
    };

    std::vector<BedStatus> statuses;
    for (const auto& bed_instance_ref : instances) {
        SlicingId slicing_id{project_id, bed_instance_ref.get().id().id};
        const auto status{m_project_interactor->status_cache().get_status(slicing_id)};
        if (status) {
            using Biz::Slicing::Error;
            std::vector<std::string> errors;
            for (const Error& error : status->errors) {
                errors.push_back(to_display_string(error, project));
            }

            using Biz::Slicing::Warning;
            std::vector<std::string> warnings;
            for (const Warning& warning : status->warrnings) {
                warnings.push_back(to_display_string(warning, project));
            }
            statuses.push_back(
                BedStatus{
                    .slicing_id = slicing_id,
                    .bed_index  = bed_instance_ref.get().index(),
                    .status     = status->code,
                    .errors     = errors,
                    .warrnings  = warnings
                }
            );
        } else {
            statuses.push_back(
                BedStatus{
                    .slicing_id = slicing_id,
                    .bed_index  = bed_instance_ref.get().index(),
                    .status     = StatusCode::InvalidData,
                    .errors     = {"Missing status!"},
                    .warrnings  = {}
                }
            );
        }
    }

    if (statuses.empty()) {
        return;
    }

    const bool any_invalid{std::ranges::any_of(statuses, [](const auto& bed_status) {
        return bed_status.status == StatusCode::InvalidData;
    })};

    const bool any_modified{std::ranges::any_of(statuses, [](const auto& bed_status) {
        return bed_status.status == StatusCode::Modified;
    })};

    const bool any_running{std::ranges::any_of(statuses, [](const auto& bed_status) {
        return bed_status.status == StatusCode::Running;
    })};

    m_button_slice->callbacks().action = []() {
    };
    m_button_slice->set_enabled(true);

    std::string label;
    std::string tooltip;
    Render::Icon icon = Render::Icon::None;
    m_navigation_button->set_visible(true);
    ImColor button_color = color_primary;

    if (any_invalid) {
        label        = _u8L("Invalid settings");
        button_color = color_error;
        icon         = Render::Icon::EyeOpen;

        std::string error_mesage;
        for (const BedStatus& bed_status : statuses) {
            if (bed_status.status == StatusCode::InvalidData) {
                std::string bed_error;
                for (const std::string& error : bed_status.errors) {
                    bed_error += fmt::format("Bed {}:\n", bed_status.bed_index);
                    bed_error += error + "\n";
                }
                bed_error += bed_error.empty() ? "" : "\n";
                error_mesage += bed_error;
            }
        }

        m_button_slice->callbacks().action = [error_mesage, label]() {
            AppServices::instance().dialog_manager().show_error_dialog(error_mesage, label);
        };
    } else if (any_modified) {
        std::string warning_tooltip;
        for (const BedStatus& bed_status : statuses) {
            if (bed_status.status == StatusCode::Modified) {
                std::string bed_warning;
                for (const std::string& warning : bed_status.warrnings) {
                    bed_warning += fmt::format("Bed {}:\n", bed_status.bed_index);
                    bed_warning += warning + "\n";
                }
                bed_warning += bed_warning.empty() ? "" : "\n";
                tooltip += bed_warning;
            }
        }

        if (!tooltip.empty()) {
            icon = Render::Icon::WarningMarkerWhite;
        }

        label = _u8L("Slice");

        m_button_slice->callbacks().action = [this, statuses]() {
            for (const BedStatus& bed_status : statuses) {
                if (bed_status.status == StatusCode::Modified) {
                    m_project_interactor->slicing_interactor().slice_bed(bed_status.slicing_id);
                }
            }
            navigate_to_other();
        };
    } else if (any_running) {
        label                              = _u8L("Cancel");
        m_button_slice->callbacks().action = [this, statuses]() {
            for (const BedStatus& bed_status : statuses) {
                if (bed_status.status == StatusCode::Running) {
                    m_project_interactor->slicing_interactor().stop_slicing_bed(bed_status.slicing_id);
                }
            }
            navigate_to_other();
        };
    } else {
        m_navigation_button->set_visible(false);
        label                              = _u8L("Preview");
        m_button_slice->callbacks().action = [this]() {
            navigate_to_other();
        };
        button_color = color_secondary;
    }

    m_button_slice->set_icon(icon);
    m_button_slice->set_label(label);
    m_button_slice->set_tooltip(tooltip);
    m_button_slice->set_background_color(button_color);
}

} // namespace Slic3r::App::Plater
