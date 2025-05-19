#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/ExportPathSelect.hpp"

namespace Slic3r::App::Preview {

constexpr float navig_btn_width = 40.f;
constexpr float export_button_size = 25;

SidebarPreviewActionButtons::SidebarPreviewActionButtons(Item* parent)
    : SidebarActionButtons("sidebar_preview_action_buttons", Render::ModuleType::Preview)
{
    set_min_size({220, 0});
    set_orientation(Yoga::Orientation::Vertical);
    set_gap(5);

    m_layout_top = new Yoga::Item(this);
    m_layout_top->set_orientation(Yoga::Orientation::Horizontal);
    m_layout_top->set_justify_content(YGJustify::YGJustifySpaceAround);
    m_layout_top->set_max_size({YGUndefined, export_button_size});

    m_button_save_print = new Yoga::LayoutButton("", ImGui::SavePrint, "Export", m_layout_top);
    m_button_save_print_to_flash =
        new Yoga::LayoutButton("", ImGui::SavePrintToFlash, "Export to flash", m_layout_top);
    m_button_save_print_to_local =
        new Yoga::LayoutButton("", ImGui::SavePrintToLocal, "Export to local", m_layout_top);
    m_button_save_print_add_bookmark =
        new Yoga::LayoutButton("", ImGui::SavePrintAddBookmark, "Upload", m_layout_top);

    m_button_save_print->set_background_color(IM_COL32_BLACK_TRANS);
    m_button_save_print_to_flash->set_background_color(IM_COL32_BLACK_TRANS);
    m_button_save_print_to_local->set_background_color(IM_COL32_BLACK_TRANS);
    m_button_save_print_add_bookmark->set_background_color(IM_COL32_BLACK_TRANS);

    m_button_save_print->callbacks().action = [this]() {
        GCodeExportPathSelect export_path_select(true);
        export_path_select.show_modal_dialog(m_project_interactor->last_export_path(false), m_project_interactor->get_project_name(m_project_interactor->selected_project_id()), 
            [this](bool result, const boost::filesystem::path& file_path) {
            if (result) {
                m_project_interactor->do_export(m_project_interactor->selected_bed_slicing_id(),file_path, false);
            }
            
        });
    };
    m_button_save_print_to_flash->callbacks().action = [this]() {
        GCodeExportPathSelect export_path_select(true);
        export_path_select.show_modal_dialog(m_project_interactor->last_export_path(true), m_project_interactor->get_project_name(m_project_interactor->selected_project_id()), 
            [this](bool result, const boost::filesystem::path& file_path) {
            if (result) {
                m_project_interactor->do_export(m_project_interactor->selected_bed_slicing_id(), file_path, true);
            }
        });
    };
    m_button_save_print_to_local->callbacks().action = [this]() {
        GCodeExportPathSelect export_path_select(true);
        export_path_select.show_modal_dialog(m_project_interactor->last_export_path(false), m_project_interactor->get_project_name(m_project_interactor->selected_project_id()), 
            [this](bool result, const boost::filesystem::path& file_path) {
            if (result) {
                m_project_interactor->do_export(m_project_interactor->selected_bed_slicing_id(), file_path, true);
            }
        });
    };
        
    m_button_save_print_add_bookmark->callbacks().action = [this]() {
         m_project_interactor->do_upload(m_project_interactor->selected_bed_slicing_id(), "filename.gcode");
    };

    m_layout_bottom = new Yoga::Item(this);
    m_layout_bottom->set_orientation(Yoga::Orientation::Horizontal);
    m_layout_bottom->set_gap(5);

    m_button_navigation =
        new Yoga::LayoutButton(m_navigator_name, '\0', m_navigator_tooltip, m_layout_bottom);
    m_button_navigation->set_background_color(color_secondary);
    m_button_navigation->set_label_font_type(Render::ImguiFontType::Bold);
    m_button_navigation->set_min_size({navig_btn_width, button_height});

    m_button_navigation->callbacks().action = [this]() {
        navigate_to_other();
    };

    m_button_print = new Yoga::LayoutButton("Print", '\0', "Print results", m_layout_bottom);
    m_button_print->set_label_font_type(Render::ImguiFontType::Bold);
    m_button_print->set_background_color(color_primary);
    m_button_print->set_min_size({0, button_height});
    m_button_print->set_flex_grow(1);
}

void SidebarPreviewActionButtons::render_body(Yoga::Vec2f pos, Yoga::Vec2f size)
{
    m_layout_top->set_enabled(export_allowed());

    SidebarActionButtons::render_body(pos, size);
}

bool SidebarPreviewActionButtons::export_allowed() const
{
    const Biz::Slicing::SlicingId id = m_project_interactor->selected_bed_slicing_id();
    const std::optional<Biz::Slicing::Status> status{
        m_project_interactor->status_cache().get_status(id)
    };

    return status && status == Biz::Slicing::Status::Finished;
}
} // namespace Slic3r::App::Preview
