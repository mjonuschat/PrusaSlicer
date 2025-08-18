#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/ExportPathSelect.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/Browser/BrowserLogicConnectSelect.hpp"
#include <Slic3r/Biz/Platform/PlatformServices.hpp>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Preview {

constexpr float navig_btn_width    = 40.f;
constexpr float export_button_size = 25;

SidebarPreviewActionButtons::SidebarPreviewActionButtons(Navigator* render_module_navigator) :
    SidebarActionButtons("sidebar_preview_action_buttons", Render::ModuleType::Preview, render_module_navigator)
{
    set_min_size({220, 0});
    set_orientation(Orientation::Vertical);
    set_gap(5);

    m_layout_top = emplace_back<Item>();
    m_layout_top->set_orientation(Orientation::Horizontal);
    m_layout_top->set_justify_content(YGJustify::YGJustifySpaceAround);
    m_layout_top->set_max_size({YGUndefined, export_button_size});

    m_button_save_print = m_layout_top->emplace_back<LayoutButton>("", Render::Icon::SavePrint, "Export");
    m_button_save_print_to_flash = m_layout_top->emplace_back<LayoutButton>("", Render::Icon::SavePrintToFlash, "Export to flash");
    m_button_save_print_to_local = m_layout_top->emplace_back<LayoutButton>("", Render::Icon::SavePrintToLocal, "Export to local");
    m_button_save_print_add_bookmark = m_layout_top->emplace_back<LayoutButton>("", Render::Icon::SavePrintAddBookmark, "Upload");

    m_button_save_print->set_background_color(IM_COL32_BLACK_TRANS);
    m_button_save_print_to_flash->set_background_color(IM_COL32_BLACK_TRANS);
    m_button_save_print_to_local->set_background_color(IM_COL32_BLACK_TRANS);
    m_button_save_print_add_bookmark->set_background_color(IM_COL32_BLACK_TRANS);

    m_button_save_print->callbacks().action = [this]()
    {
        Biz::Platform::PlatformServices::instance().main_thread_dispatcher().dispatch_on_main_thread_after(
            [this]()
            {
                GCodeExportPathSelect export_path_select(true);
                export_path_select.show_modal_dialog(
                    m_project_interactor->last_export_path(false),
                    m_project_interactor->get_project_name(m_project_interactor->selected_project_id()),
                    [this](bool result, const std::vector<boost::filesystem::path>& file_paths)
                    {
                        if (result) {
                            m_project_interactor->do_export(
                                m_project_interactor->selected_bed_slicing_id(),
                                file_paths.front()
                            );
                        }
                    }
                );
            }
        );
    };
    m_button_save_print_to_flash->callbacks().action = [this]()
    {
        Biz::Platform::PlatformServices::instance().main_thread_dispatcher().dispatch_on_main_thread_after(
            [this]()
            {
                GCodeExportPathSelect export_path_select(true);
                export_path_select.show_modal_dialog(
                    m_project_interactor->removable_drive_service()
                        .get_path_on_removable_drive(m_project_interactor->last_export_path(true)),
                    m_project_interactor->get_project_name(m_project_interactor->selected_project_id()),
                    [this](bool result, const std::vector<boost::filesystem::path>& file_paths)
                    {
                        if (result) {
                            m_project_interactor->do_export(
                                m_project_interactor->selected_bed_slicing_id(),
                                file_paths.front()
                            );
                        }
                    }
                );
            }
        );
    };
    m_button_save_print_to_local->callbacks().action = [this]()
    {
        Biz::Platform::PlatformServices::instance().main_thread_dispatcher().dispatch_on_main_thread_after(
            [this]()
            {
                GCodeExportPathSelect export_path_select(true);
                export_path_select.show_modal_dialog(
                    m_project_interactor->last_export_path(false),
                    m_project_interactor->get_project_name(m_project_interactor->selected_project_id()),
                    [this](bool result, const std::vector<boost::filesystem::path>& file_paths)
                    {
                        if (result) {
                            m_project_interactor->do_export(
                                m_project_interactor->selected_bed_slicing_id(),
                                file_paths.front()
                            );
                        }
                    }
                );
            }
        );
    };

    m_button_save_print_add_bookmark->callbacks().action = [this]()
    {
        Biz::Platform::PlatformServices::instance().main_thread_dispatcher().dispatch_on_main_thread_after(
            [this]()
            {
                AppServices::instance().dialog_manager().show_webview_dialog(std::make_unique<Browser::BrowserLogicConnectSelect>(*m_project_interactor), m_project_interactor);
                // m_project_interactor->do_upload(m_project_interactor->selected_bed_slicing_id(), "filename.gcode");
            }
        );
    };

    m_layout_bottom = emplace_back<Item>();
    m_layout_bottom->set_orientation(Orientation::Horizontal);
    m_layout_bottom->set_gap(5);

    m_button_navigation = m_layout_bottom->emplace_back<LayoutButton>(m_navigator_name, Render::Icon::None, m_navigator_tooltip);
    m_button_navigation->set_background_color(color_secondary);
    m_button_navigation->set_label_font_type(Render::ImguiFontType::Bold);
    m_button_navigation->set_min_size({navig_btn_width, button_height});

    m_button_navigation->callbacks().action = [this]() { navigate_to_other(); };

    m_button_print = m_layout_bottom->emplace_back<LayoutButton>("Print", Render::Icon::None, "Print results");
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
    const Domain::SlicingId id = m_project_interactor->selected_bed_slicing_id();
    const std::optional<Biz::Slicing::Status> status{m_project_interactor->status_cache().get_status(id)};

    return status && status->code == Biz::Slicing::StatusCode::Finished;
}

} // namespace Slic3r::App::Preview
