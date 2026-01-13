#include "Slic3r/App/TopBar.hpp"

#include "Slic3r/App/Yoga/ProjectButton.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Menu.hpp"
#include "Slic3r/App/Yoga/MenuItem.hpp"

#include "Slic3r/App/SearchBar.hpp"
#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/ThumbnailStore.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Wildcards.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/MenuItem.hpp"
#include "Slic3r/App/MenuBuilder.hpp"
#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/CommandBindingManager.hpp"
#include "Slic3r/App/Platform/CommandName.hpp"
#include "Slic3r/App/UIItemCommand.hpp"
#include "Slic3r/App/AppConfig.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

using namespace Yoga;
using namespace Slic3r::Biz;

using CommandName = Platform::CommandName;

TopBar::TopBar(
    Biz::ProjectInteractor* project_interactor,
    Platform::AbstractRenderModule* render_module,
    ThumbnailStore& thumbnail_store,
    Navigator& navigator
) :
    Window("TopBar"),
    m_selected_project_changed_listener_scope(*project_interactor, *this),
    m_project_interactor(*project_interactor),
    m_render_module(render_module),
    m_thumbnail_store(thumbnail_store),
    m_navigator(navigator)
{
    register_menu_commands();

    Paddings paddings = padding();

    set_padding(0);
    set_alpha(0.f);
    set_rounding(0.f);
    set_gap(0.f);
    set_flex_shrink(0);

    Rectangle* left_wrapper = emplace_back<Rectangle>();
    left_wrapper->set_flex_shrink(0);

    add_menu_btns(left_wrapper);
    add_save_project_btn(left_wrapper);
    add_show_ui_btn(left_wrapper);

    m_list_view = emplace_back<ProjectButtonListView>(m_project_interactor);
    m_list_view->set_window_flags(
        ImGuiWindowFlags_HorizontalScrollbar // compute horizontal scroll range
        | ImGuiWindowFlags_NoScrollbar // don't show any scrollbar
        | ImGuiWindowFlags_NoScrollWithMouse // don't let ImGui consume wheel vertically
    );
    m_list_view->set_remap_horizontal_scroll(true);
    m_list_view->set_source_list(&project_interactor->observable_project_list());

    Rectangle* project_actions_wrapper = emplace_back<Rectangle>();
    project_actions_wrapper->set_flex_grow(1.);

    // add_new_project_btn(project_actions_wrapper);
    // add_expander_btn(project_actions_wrapper);

    Rectangle* right_wrapper = emplace_back<Rectangle>();
    right_wrapper->set_flex_shrink(0);
    m_search_bar = right_wrapper->emplace_back<SearchBar>(m_project_interactor, m_navigator);

    for (LayoutButton* btn :
         std::initializer_list<LayoutButton*>{
             m_save_btn,
             m_show_ui_btn,
             m_main_menu_btn,
             m_file_menu_btn,
         })
    {
        if (!btn)
            continue;
        btn->set_background_color(IM_COL32_BLACK_TRANS);
        btn->set_tooltip_position(Position::Bottom);
    }

    for (Rectangle* wrapper :
         std::initializer_list<Rectangle*>{left_wrapper, right_wrapper, project_actions_wrapper})
    {
        wrapper->set_fill(GImGui->Style.Colors[ImGuiCol_WindowBg]);
        wrapper->set_rounding(0.f);
        wrapper->set_gap(15.f);
        wrapper->set_padding(paddings);
    }
}

void TopBar::on_selected_project_changed(size_t index)
{
    m_list_view->scroll_at_item(m_list_view->get_item(index));
}

void TopBar::focus_search()
{
    m_search_bar->focus_search();
}

void TopBar::load_project()
{
    IDialogManager::FileCallback callback =
        [this](bool success, const std::vector<boost::filesystem::path>& file_paths)
    {
        if (success) {
            m_project_interactor.load_project(file_paths.front());
        }
    };

    auto& dlg_manager = AppServices::instance().dialog_manager();
    dlg_manager.show_file_dialog(
        FileDialogType::Open,
        _u8L("Open Project"),
        m_project_interactor.project_dir(
            m_project_interactor.selected_project_id(),
            AppServices::instance().app_config().get<std::string>("last_used_directory")
        ),
        "",
        Wildcards::generate_wildcards(Wildcards::TypeFlag::Project3mf),
        callback
    );
}

void TopBar::save_project_as()
{
    auto& dlg_manager = AppServices::instance().dialog_manager();
    dlg_manager.show_yesno_dialog(
        "DEVELOPER WARNING",
        "EXPORT TO 3MF IS NOT FINALIZED YET.\n\nThe exported project MUST NOT be shared publicly, "
        "it will not be compatible with both old PrusaSlicer and the finalized 3.0.0.\n\n"
        "Do you really want to export it?",
        [this](bool answer)
        {
            if (!answer)
                return;
            Domain::SelectionId selected_project_id = m_project_interactor.selected_project_id();
            const std::string& project_name =
                m_project_interactor.get_project_name(selected_project_id);
            Store3mfParam params{
                .thumbnail = m_thumbnail_store.projects.selected().thumbnail_3mf.get()
            };
            if (true
                || project_name
                       .empty()) { // The 'true' is here for the development phase - effectively it always "Saves as".
                // Saving a new project - show file save dialog.
                IDialogManager::FileCallback callback =
                    [this,
                     &params](bool success, const std::vector<boost::filesystem::path>& file_paths)
                {
                    if (success) {
                        boost::filesystem::path file_path = file_paths.front();
                        std::string ext_str               = file_path.extension().string();
                        // file path could have locale dependent characters, do not use tolower
                        if (ext_str != ".3mf" && ext_str != ".3MF") {
                            file_path.replace_extension(".3mf");
                        }

                        m_project_interactor.save_project(file_path, params);
                    }
                };
                if (true
                    || project_name
                           .empty()) { // The 'true' is here for the development phase - effectively it always "Saves as".
                    // Saving a new project - show file save dialog.
                    IDialogManager::FileCallback callback =
                        [this, &params](
                            bool success,
                            const std::vector<boost::filesystem::path>& file_paths
                        )
                    {
                        if (success)
                            m_project_interactor.save_project(file_paths.front(), params);
                    };
                    auto& dlg_manager = AppServices::instance().dialog_manager();
                    dlg_manager.show_file_dialog(
                        FileDialogType::Save,
                        _u8L("Save Project"),
                        m_project_interactor.project_dir(
                            m_project_interactor.selected_project_id(),
                            AppServices::instance().app_config().get<std::string>("last_used_directory")
                        ),
                        project_name,
                        Wildcards::generate_wildcards(Wildcards::TypeFlag::Project3mf),
                        callback
                    );
                } else {
                    // Saving an existing project - just save.
                    // DK: How this could work with just project_name?
                    m_project_interactor.save_project(
                        boost::filesystem::path(project_name),
                        params
                    );
                }
            }
        }
    );
}

void TopBar::add_save_project_btn(Item* parent)
{
    m_save_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarSave, _u8L("Save"));
    m_render_module->command_binding_manager().bind_tb_item(CommandName::SaveProject, m_save_btn);
}

void TopBar::add_show_ui_btn(Item* parent)
{
    m_show_ui_btn =
        parent->emplace_back<LayoutButton>("", Render::Icon::TobBarShowUI, _u8L("Hide sidebars"));
    m_show_ui_btn->set_checkable(true);

    m_show_ui_btn->callbacks().action = [this]()
    {
        m_show_ui_btn->set_tooltip(
            m_show_ui_btn->checked() ? _u8L("Show sidebars") : _u8L("Hide sidebars")
        );
        // Propagate sidebars visibility into active RenderModule
        m_render_module->set_sidebars_visible(!m_show_ui_btn->checked());

        // ysTODO: save hide value into app_config
    };
}

void TopBar::add_menu_btns(Item* parent)
{
    MenuBuilder menu_builder(
        m_render_module->menu_manager(),
        m_render_module->command_binding_manager()
    );

    if (App::MenuItem* main_menu_item =
            m_render_module->menu_manager().menu_item(MenuItemName::MainMenu))
    {
        m_main_menu_btn = parent->emplace_back<LayoutButton>(
            MenuBuilder::item_name_translated(main_menu_item->name()),
            MenuBuilder::item_icon(main_menu_item->name())
        );
        m_main_menu_btn->set_background_color(IM_COL32_BLACK_TRANS);
        m_main_menu_btn->callbacks().action = [this]() { m_main_menu->open(); };

        m_main_menu =
            m_main_menu_btn->emplace_back<Yoga::Menu>("main_menu", Yoga::Position::Bottom);
        menu_builder.add_menu_items(m_main_menu, main_menu_item);
    }

    if (App::MenuItem* file_menu_item =
            m_render_module->menu_manager().menu_item(MenuItemName::FileMenu))
    {
        m_file_menu_btn = parent->emplace_back<LayoutButton>(
            MenuBuilder::item_name_translated(file_menu_item->name()),
            MenuBuilder::item_icon(file_menu_item->name())
        );
        m_file_menu_btn->set_background_color(IM_COL32_BLACK_TRANS);
        m_file_menu_btn->callbacks().action = [this]() { m_file_menu->open(); };

        m_file_menu =
            m_file_menu_btn->emplace_back<Yoga::Menu>("file_menu", Yoga::Position::Bottom);
        menu_builder.add_menu_items(m_file_menu, file_menu_item);
    }
}

void TopBar::register_menu_commands()
{
    m_render_module
        ->menu_manager()

        // Menu -> Edit -> Deselect All
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::DeselectAll},
            std::make_unique<UIItemCommand>(
                CommandName::ClearSelection,
                [this]()
                {
                    auto& scene_interactor = m_project_interactor.scene_interactor();
                    if (!scene_interactor.object_selection().empty())
                        scene_interactor.clear_object_selection();
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut = Platform::KeyboardShortcut{0, Platform::KeyCode::Escape},
                    .enabled           = [this]()
                    { return !m_project_interactor.scene_interactor().object_selection().empty(); }
                }
            )
        )
        // Menu -> Edit -> Deleet Selected
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::DeleteSelected},
            std::make_unique<UIItemCommand>(
                CommandName::DeleteSelected,
                [this]()
                {
                    if (m_project_interactor.scene_interactor().object_selection().empty())
                        return;

                    std::optional<std::string> last_solid_part_name =
                        m_project_interactor.scene_interactor().delete_selected_elements();

                    if (last_solid_part_name) {
                        // Show warning dialog
                        auto& dlg_manager = App::AppServices::instance().dialog_manager();
                        dlg_manager.show_warning_dialog(
                            fmt::vformat(
                                _u8L(
                                    "Part {} could not be deleted from the object,\n"
                                    "as removing the last solid part is not permitted."
                                ),
                                fmt::make_format_args(last_solid_part_name.value())
                            ) + "\n",
                            _u8L("Delete selection")
                        );
                    }
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut = Platform::KeyboardShortcut{0, Platform::KeyCode::Delete},
                    .enabled           = [this]()
                    { return !m_project_interactor.scene_interactor().object_selection().empty(); }
                }
            )
        )

        // Menu -> Edit -> Search
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::Search},
            std::make_unique<UIItemCommand>(
                CommandName::Search,
                [this]() { m_navigator.request_search(); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::F
                        }
                }
            )
        )

        // Menu -> Preferences
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Preferences},
            std::make_unique<UIItemCommand>(
                CommandName::Preferences,
                [this]() { m_navigator.set_opened_preferences(true); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::P
                        }
                }
            )
        )

        // Menu -> New project
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::NewProject},
            std::make_unique<UIItemCommand>(
                CommandName::NewProject,
                [this]() { m_project_interactor.new_project(); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::N
                        }
                }
            )
        )

        // Menu -> New project
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::OpenProject},
            std::make_unique<UIItemCommand>(
                CommandName::OpenProject,
                [this]() { load_project(); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::O
                        }
                }
            )
        )

        // Menu -> Save project
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::SaveProject},
            std::make_unique<UIItemCommand>(
                CommandName::SaveProject,
                [this]()
                {
                    // ToDo: implement save project function
                    /*save_project();*/
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::S
                        }
                }
            )
        )

        // Menu -> Save project as
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::SaveProjectAs},
            std::make_unique<UIItemCommand>(
                CommandName::SaveProjectAs,
                [this]() { save_project_as(); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl)
                                | Platform::KeyModifiers(Platform::KeyModifier::Alt),
                            Platform::KeyCode::S
                        }
                }
            )
        )
            // File -> Import -> children
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::Import, MenuItemName::ImportGeometry},
            std::make_unique<UIItemCommand>(
                CommandName::AddObject,
                [this]()
                {
                    IDialogManager::FileCallback callback =
                        [this](bool success, const std::vector<boost::filesystem::path>& file_paths)
                    {
                        if (success) {
                            const auto& proj            = m_project_interactor.selected_project();
                            Domain::BedRef selected_bed = m_project_interactor.scene_interactor()
                                                              .bed_selection()
                                                              .last_selected_bed();
                            const Domain::ConfigContainer* cc =
                                proj.find_config_container(selected_bed.config_container_id);
                            const Domain::BedInstance& inst =
                                cc->find_bed_instance(selected_bed.instance_id);
                            int nozzle_dmrs_cnt = cc->selected_preset().hw_config.tool_count;
                            Biz::FileLoadingLogic::import_files_and_add_to_scene(
                                file_paths,
                                nozzle_dmrs_cnt,
                                m_project_interactor.scene_interactor(),
                                cc->bed().center()
                                    + Biz::Algorithms::Point::to_2d(
                                        inst.transformation.get_offset()
                                    ),
                                &App::AppServices::instance().dialog_manager()
                            );

                            m_navigator.navigate_to_module_type(App::Render::ModuleType::Plater);

                            m_project_interactor.set_project_dir(
                                m_project_interactor.selected_project_id(),
                                file_paths.front()
                            );
                        }
                    };


                    auto& dlg_manager = App::AppServices::instance().dialog_manager();
                    dlg_manager.show_file_dialog(
                        FileDialogType::OpenMultiple,
                        _u8L("Import File"),
                        m_project_interactor.project_dir(
                            m_project_interactor.selected_project_id(),
                            AppServices::instance().app_config().get<std::string>("last_used_directory")
                        ),
                        "",
                        Wildcards::generate_wildcards(
                            Wildcards::TypeFlag::Project3mf | Wildcards::TypeFlag::Stl | Wildcards::TypeFlag::Obj | Wildcards::TypeFlag::Step,
                            Wildcards::TypeFlag::AllImportFiles
                        ),
                        callback
                    );
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut = Platform::KeyboardShortcut{
                        Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                        Platform::KeyCode::I
                    }
                }
            )
        );
}

} // namespace Slic3r::App
