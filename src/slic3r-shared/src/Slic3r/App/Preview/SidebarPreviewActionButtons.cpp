#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"

#include "Slic3r/App/Browser/BrowserLogicLogInRedirect.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/ExportPathSelect.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/Browser/BrowserLogicConnectSelect.hpp"
#include <Slic3r/Biz/Platform/PlatformServices.hpp>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Preview {

using Biz::Platform::IMainThreadDispatcher;
using Biz::Platform::PlatformServices;

namespace {

constexpr float secondary_button_size = 35.f;

void style_secondary_button(LayoutButton* button)
{
    button->set_min_size({secondary_button_size, secondary_button_size});
    button->set_background_color(IM_COL32_BLACK_TRANS);
}

std::function<void()> get_export_action(Biz::ProjectInteractor* project_interactor)
{
    auto call_do_export{
        [=]()
        {
            GCodeExportPathSelect export_path_select(true);
            export_path_select.show_modal_dialog(
                project_interactor->last_export_path(false),
                project_interactor->get_project_name(project_interactor->selected_project_id()),
                [&](bool result, const std::vector<boost::filesystem::path>& file_paths)
                {
                    if (result) {
                        project_interactor->do_export(
                            project_interactor->selected_bed_slicing_id(),
                            file_paths.front()
                        );
                    }
                }
            );
        }
    };

    return [=]()
    {
        IMainThreadDispatcher& dispatcher{PlatformServices::instance().main_thread_dispatcher()};
        if (!dispatcher.dispatch_on_main_thread_after(call_do_export)) {
            SPDLOG_INFO("Export request not dispatched!");
        }
    };
}

std::function<void()> get_export_flash_action(Biz::ProjectInteractor* project_interactor)
{
    auto call_do_export{
        [=]()
        {
            GCodeExportPathSelect export_path_select(true);
            export_path_select.show_modal_dialog(
                project_interactor->removable_drive_service().get_path_on_removable_drive(
                    project_interactor->last_export_path(true)
                ),
                project_interactor->get_project_name(project_interactor->selected_project_id()),
                [=](bool result, const std::vector<boost::filesystem::path>& file_paths)
                {
                    if (result) {
                        project_interactor->do_export(
                            project_interactor->selected_bed_slicing_id(),
                            file_paths.front()
                        );
                    }
                }
            );
        }
    };

    return [=]()
    {
        IMainThreadDispatcher& dispatcher{PlatformServices::instance().main_thread_dispatcher()};
        if (!dispatcher.dispatch_on_main_thread_after(call_do_export)) {
            SPDLOG_INFO("Export request not dispatched!");
        }
    };
}

std::function<void()> get_send_to_connect_action(Biz::ProjectInteractor* project_interactor)
{
    auto send_to_connect{
        [=]()
        {
            AppServices::instance().dialog_manager().show_webview_dialog(
                std::make_unique<Browser::BrowserLogicConnectSelect>(*project_interactor),
                project_interactor
            );
        }
    };
    return [=]()
    {
        IMainThreadDispatcher& dispatcher{PlatformServices::instance().main_thread_dispatcher()};
        if (!dispatcher.dispatch_on_main_thread_after(send_to_connect)) {
            SPDLOG_INFO("Send to connect not dispatched!");
        };
    };
}

const std::string export_tooltip{"Export gcode to a file"};

std::unique_ptr<LayoutButton> get_export_button(Biz::ProjectInteractor* project_interactor)
{
    auto result{std::make_unique<LayoutButton>("", Render::Icon::SavePrint, export_tooltip)};
    style_secondary_button(result.get());
    result->callbacks().action = get_export_action(project_interactor);

    return result;
}

std::unique_ptr<LayoutButton> get_export_flash_button(Biz::ProjectInteractor* project_interactor)
{
    auto result{std::make_unique<LayoutButton>(
        "",
        Render::Icon::SavePrintToFlash,
        "Export to a flash drive"
    )};
    style_secondary_button(result.get());
    result->callbacks().action = get_export_flash_action(project_interactor);
    return result;
}

std::unique_ptr<LayoutButton> get_send_directly_button(Biz::ProjectInteractor* project_interactor)
{
    auto result{std::make_unique<LayoutButton>(
        "",
        Render::Icon::SavePrintToLocal,
        "Send directly to a printer\nAdd a physical printer to enable.\n(WIP)"
    )};
    style_secondary_button(result.get());
    result->set_enabled(false);
    return result;
}

void style_layout(Item& layout, float gap)
{
    layout.set_min_size({220, 0});
    layout.set_orientation(Orientation::Vertical);
    layout.set_gap(gap);
    layout.set_flex_grow(1);
}

ItemPtr get_layout(
    Biz::ProjectInteractor& project_interactor,
    ItemPtr navigation_button,
    std::vector<std::unique_ptr<LayoutButton>> secondary_buttons,
    std::unique_ptr<LayoutButton> primary_button,
    float gap,
    float navig_btn_width
)
{
    auto layout{std::make_unique<Item>()};
    style_layout(*layout, gap);

    auto secondary_buttons_layout{layout->emplace_back<Item>()};
    secondary_buttons_layout->set_gap(gap);
    secondary_buttons_layout->set_flex_grow(1);
    secondary_buttons_layout->set_orientation(Orientation::Horizontal);
    secondary_buttons_layout->set_justify_content(YGJustify::YGJustifyFlexEnd);
    secondary_buttons_layout->set_max_size({YGUndefined, secondary_button_size});

    for (auto& button : secondary_buttons) {
        secondary_buttons_layout->append(std::move(button));
    }

    auto layout_bottom{layout->emplace_back<Item>()};
    layout_bottom->set_orientation(Orientation::Horizontal);
    layout_bottom->set_gap(gap);

    layout_bottom->append(std::move(navigation_button));

    layout_bottom->append(std::move(primary_button));

    return layout;
}

std::vector<LayoutButton*> get_raw(const std::vector<std::unique_ptr<LayoutButton>>& buttons)
{
    std::vector<LayoutButton*> result;
    std::ranges::transform(
        buttons,
        std::back_inserter(result),
        [](const auto& button) { return button.get(); }
    );
    return result;
}
} // namespace

std::unique_ptr<LayoutButton> SidebarPreviewActionButtons::get_primary_button()
{
    auto result{std::make_unique<LayoutButton>("Plater", Render::Icon::None, "Back to Plater")};
    result->set_label_font_type(Render::ImguiFontType::Bold);
    result->set_background_color(color_secondary);
    result->set_min_size({0, button_height});
    result->set_flex_grow(1);
    result->callbacks().action = [this]() { navigate_to_other(); };
    return result;
}

SidebarPreviewActionButtons::SidebarPreviewActionButtons(Navigator* render_module_navigator) :
    SidebarActionButtons("sidebar_preview_action_buttons", Render::ModuleType::Preview, render_module_navigator)
{}

SidebarPreviewActionButtons::~SidebarPreviewActionButtons()
{
    m_project_interactor->user_account_interactor()
        .remove_listener<Biz::UserAccount::IUserAccountListener>(this);
    m_project_interactor->status_cache().remove_listener<Biz::IStatusCacheChangedListener>(this);
    m_project_interactor->scene_interactor()
        .remove_listener<Biz::ISelectedBedInstancesChangedListener>(this);
}

void SidebarPreviewActionButtons::on_init(Biz::ProjectInteractor* project_interactor)
{
    m_project_interactor = project_interactor;
    m_project_interactor->user_account_interactor()
        .add_listener<Biz::UserAccount::IUserAccountListener>(this);
    m_project_interactor->status_cache().add_listener<Biz::IStatusCacheChangedListener>(this);
    m_project_interactor->scene_interactor().add_listener<Biz::ISelectedBedInstancesChangedListener>(
        this
    );
    m_project_interactor->removable_drive_service().add_status_listener(this);

    const float gap{5};

    std::unique_ptr<LayoutButton> primary_button{get_primary_button()};
    m_layout_without_connect.primary_button = primary_button.get();

    std::vector<std::unique_ptr<LayoutButton>> secondary_buttons;
    secondary_buttons.emplace_back(get_send_directly_button(m_project_interactor));
    secondary_buttons.emplace_back(get_export_flash_button(m_project_interactor));

    std::unique_ptr<LayoutButton> navigation_button{get_navigation_button()};
    m_layout_without_connect.navigation_button = navigation_button.get();
    navigation_button->set_visible(false);

    m_layout_without_connect.secondary_buttons = get_raw(secondary_buttons);
    m_layout_without_connect.layout            = get_layout(
        *m_project_interactor,
        std::move(navigation_button),
        std::move(secondary_buttons),
        std::move(primary_button),
        gap,
        navig_btn_width
    );
    m_layout_without_connect.layout_raw = m_layout_without_connect.layout.get();

    primary_button                       = get_primary_button();
    m_layout_with_connect.primary_button = primary_button.get();

    secondary_buttons.clear();
    secondary_buttons.emplace_back(get_export_button(m_project_interactor));
    secondary_buttons.emplace_back(get_send_directly_button(m_project_interactor));
    secondary_buttons.emplace_back(get_export_flash_button(m_project_interactor));

    navigation_button                       = get_navigation_button();
    m_layout_with_connect.navigation_button = navigation_button.get();
    navigation_button->set_visible(false);

    m_layout_with_connect.secondary_buttons = get_raw(secondary_buttons);

    m_layout_with_connect.layout = get_layout(
        *m_project_interactor,
        std::move(navigation_button),
        std::move(secondary_buttons),
        std::move(primary_button),
        gap,
        navig_btn_width
    );
    m_layout_with_connect.layout_raw = m_layout_with_connect.layout.get();

    update_buttons();
}

void SidebarPreviewActionButtons::switch_layout(ActionButtonsLayoutType layout_type)
{
    ASSERT(m_layout_without_connect.layout || m_layout_with_connect.layout);

    switch (layout_type) {
    case ActionButtonsLayoutType::WithoutConnect: {
        if (!m_layout_without_connect.layout) {
            return;
        }
        if (!m_layout_with_connect.layout) {
            m_layout_with_connect.layout = remove_child(m_layout_with_connect.layout_raw);
        }
        ASSERT(m_children.empty());
        append(std::move(m_layout_without_connect.layout));
    } break;
    case ActionButtonsLayoutType::WithConnect: {
        if (!m_layout_with_connect.layout) {
            return;
        }
        if (!m_layout_without_connect.layout) {
            m_layout_without_connect.layout = remove_child(m_layout_without_connect.layout_raw);
        }
        ASSERT(m_children.empty());
        append(std::move(m_layout_with_connect.layout));
    } break;
    default:
        PANIC("No laytout selected layout!");
    }
}

void SidebarPreviewActionButtons::on_user_account_id_success(bool, const std::string&)
{
    update_buttons();
}

void SidebarPreviewActionButtons::on_user_account_logged_out()
{
    update_buttons();
}

ActionButtonsLayoutType SidebarPreviewActionButtons::active_layout() const
{
    if (!m_layout_without_connect.layout) {
        return ActionButtonsLayoutType::WithoutConnect;
    }

    if (!m_layout_with_connect.layout) {
        return ActionButtonsLayoutType::WithConnect;
    }

    return ActionButtonsLayoutType::None;
}

void SidebarPreviewActionButtons::on_selected_bed_instances_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::BedSelection& bed_selection
)
{
    update_buttons();
}

void SidebarPreviewActionButtons::on_status_cache_changed(const Domain::SlicingId slicing_id)
{
    const Domain::SlicingId current_id{m_project_interactor->selected_bed_slicing_id()};
    if (current_id != slicing_id) {
        return;
    }
    update_buttons();
}

void SidebarPreviewActionButtons::on_removable_drive_status_changed(
    const boost::filesystem::path&,
    Biz::RemovableDrive::RemovableDriveStatus s
)
{
    update_buttons();
}

void SidebarPreviewActionButtons::update_buttons()
{
    const bool logged_in{m_project_interactor->user_account_interactor().is_logged_in()};
    switch_layout(
        logged_in ? ActionButtonsLayoutType::WithConnect : ActionButtonsLayoutType::WithoutConnect
    );

    const Domain::SlicingId slicing_id{m_project_interactor->selected_bed_slicing_id()};
    const auto optional_status{m_project_interactor->status_cache().get_status(slicing_id)};
    if (!optional_status) {
        return;
    }
    const ActionButtonsLayoutType layout_type{active_layout()};
    if (layout_type == ActionButtonsLayoutType::None) {
        return;
    }

    const Biz::Slicing::Status status{*optional_status};

    LayoutButton* primary_button{
        layout_type == ActionButtonsLayoutType::WithoutConnect ?
            m_layout_without_connect.primary_button :
            m_layout_with_connect.primary_button
    };

    const std::vector<LayoutButton*>& secondary_buttons{
        layout_type == ActionButtonsLayoutType::WithoutConnect ?
            m_layout_without_connect.secondary_buttons :
            m_layout_with_connect.secondary_buttons
    };

    LayoutButton* navigation_button{
        layout_type == ActionButtonsLayoutType::WithoutConnect ?
            m_layout_without_connect.navigation_button :
            m_layout_with_connect.navigation_button
    };

    primary_button->set_background_color(color_primary);
    primary_button->callbacks().action = []() {};
    navigation_button->set_visible(true);
    for (LayoutButton* button : secondary_buttons) {
        button->set_enabled(false);
        button->callbacks().action = []() {};
    }

    using Biz::Slicing::StatusCode;
    switch (status.code) {
    case StatusCode::Running: {
        primary_button->set_label("Cancel");
        primary_button->set_enabled(true);
        primary_button->callbacks().action = [this, slicing_id]()
        { m_project_interactor->slicing_interactor().stop_slicing_bed(slicing_id); };
    } break;
    case StatusCode::InvalidData: {
        primary_button->set_label("Invalid settings");
        primary_button->set_background_color(color_error);
        primary_button->set_enabled(true);

        const std::string error{status.error.empty() ? "Unknown issue" : status.error};

        primary_button->callbacks().action = [error]()
        { AppServices::instance().dialog_manager().show_error_dialog(error, "Invalid settings"); };
    } break;
    case StatusCode::Finished: {
        if (layout_type == ActionButtonsLayoutType::WithoutConnect) {
            primary_button->set_label("Export");
            primary_button->set_enabled(true);
            primary_button->set_tooltip(export_tooltip);
            primary_button->callbacks().action = get_export_action(m_project_interactor);

            if (m_project_interactor->removable_drive_service().has_removable_drives()) {
                secondary_buttons.at(1)->set_enabled(true);
                secondary_buttons.at(1)->callbacks().action = get_export_flash_action(m_project_interactor);
            }

        } else if (layout_type == ActionButtonsLayoutType::WithConnect) {
            primary_button->set_label("Send to Connect");
            primary_button->set_tooltip("Send to Connect");
            primary_button->callbacks().action = get_send_to_connect_action(m_project_interactor);

            secondary_buttons.at(0)->set_enabled(true);
            secondary_buttons.at(0)->callbacks().action = get_export_action(m_project_interactor);
            if (m_project_interactor->removable_drive_service().has_removable_drives()) {
                secondary_buttons.at(2)->set_enabled(true);
                secondary_buttons.at(2)->callbacks().action = get_export_flash_action(m_project_interactor);
            }
        } else {
            PANIC("Unreachable!");
        }
    } break;
    case StatusCode::Modified: {
        primary_button->set_label("Slice");
        primary_button->set_enabled(true);
        primary_button->set_tooltip("Slice");
        primary_button->callbacks().action = [this, slicing_id]()
        { m_project_interactor->slicing_interactor().slice_bed(slicing_id); };
    } break;
    default: {
        primary_button->set_label("Plater");
        primary_button->set_tooltip("Back to Plater");
        primary_button->set_background_color(color_secondary);
        primary_button->set_enabled(true);
        primary_button->callbacks().action = [this]() { navigate_to_other(); };

        navigation_button->set_visible(false);
    } break;
    }
}

void SidebarPreviewActionButtons::render_body(Yoga::Vec2f pos, Yoga::Vec2f size)
{
    SidebarActionButtons::render_body(pos, size);
}

} // namespace Slic3r::App::Preview
