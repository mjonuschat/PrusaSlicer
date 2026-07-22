#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"

#include "Slic3r/App/DisplayStrings.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/IDialogManager.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace Slic3r::App::Plater {

using Biz::Scene::BedInstances;
using Biz::Scene::BedSelection;
using Biz::Slicing::StatusCode;
using Domain::SelectionId;
using Domain::SlicingId;

SidebarPlaterActionButtons::SidebarPlaterActionButtons(Navigator* render_module_navigator) :
    SidebarActionButtons("SidebarPlaterActionButtons", Render::ModuleType::Plater, render_module_navigator)
{
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

    init_physical_printer_ui();

    auto layout_bottom = m_buttons_layout->emplace_back<Item>();
    layout_bottom->set_orientation(Orientation::Horizontal);

    m_button_slice = layout_bottom->emplace_back<LayoutButton>("Slice");
    m_button_slice->set_flex_grow(1);
    m_button_slice->set_background_color(Platform::Color::AccentPrimary);
    m_button_slice->set_min_height(button_height);
    m_button_slice->set_label_font_type(Render::ImguiFontType::Bold);
    m_button_slice->set_enabled(false);
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
    const BedInstances instances{m_project_interactor->scene_interactor().selected_bed_instances()};

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

    const bool any_running{std::ranges::any_of(statuses, [](const auto& bed_status) {
        return bed_status.status == StatusCode::Running;
    })};

    const bool all_empty{std::ranges::all_of(statuses, [](const auto& bed_status) {
        return bed_status.status == StatusCode::Empty;
    })};

    m_button_slice->callbacks().action = []() {};
    m_button_slice->set_enabled(true);

    std::string label;
    std::string tooltip;
    Render::Icon icon = Render::Icon::None;
    ImColor button_color = m_theme->color_imgui(Platform::Color::AccentPrimary);

    if (any_invalid) {
        label        = _u8L("Invalid settings");
        button_color = m_theme->color_imgui(Platform::Color::Error);
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
    } else if (any_running) {
        label = _u8L("Cancel");

        m_button_slice->callbacks().action = [this, statuses]()
        {
            for (const BedStatus& bed_status : statuses) {
                if (bed_status.status == StatusCode::Running) {
                    m_project_interactor->slicing_interactor().stop_slicing_bed(
                        bed_status.slicing_id
                    );
                }
            }
        };
    } else {
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

        label = statuses.size() <= 1 ? _u8L("Slice") : _u8L("Slice all");

        m_button_slice->callbacks().action = [this, statuses]() {
            for (const BedStatus& bed_status : statuses) {
                if (bed_status.status == StatusCode::Modified) {
                    m_project_interactor->slicing_interactor().slice_bed(bed_status.slicing_id);
                }
            }
            navigate_to_other();
        };

        if (all_empty) {
            m_button_slice->set_enabled(false);
            button_color = m_theme->color_imgui(
                Platform::Color::AccentPrimary,
                Platform::ColorGroup::Disabled
            );
        }
    }

    m_button_slice->set_icon(icon);
    m_button_slice->set_label(label);
    m_button_slice->set_tooltip(tooltip);
    m_button_slice->set_background_color(button_color);
}

} // namespace Slic3r::App::Plater
