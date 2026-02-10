#include "Slic3r/App/MenuCommandRegistrar.hpp"

#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/UIItemCommand.hpp"
#include "Slic3r/App/Platform/CommandName.hpp"
#include "Slic3r/App/ResultExport/ExportActions.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/ThumbnailStore.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/AppConfigInteractor.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Wildcards.hpp"
#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/Localization.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"

namespace Slic3r::App {

#define SHOW_NOT_IMPLEMENTED_ITEMS 1

using namespace Slic3r::Biz;
using CommandName = Platform::CommandName;

MenuCommandRegistrar::MenuCommandRegistrar(
    MenuManager& menu_manager,
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator,
    ThumbnailStore& thumbnail_store
) :
    m_menu_manager(menu_manager),
    m_project_interactor(project_interactor),
    m_navigator(navigator),
    m_thumbnail_store(thumbnail_store)
{
    register_all();
}

void MenuCommandRegistrar::register_all()
{
    register_main_menu_commands();
    register_file_menu_commands();
}

void MenuCommandRegistrar::load_project()
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

void MenuCommandRegistrar::save_project()
{
    // Use "Save project as" while we don't have a project state
    save_project_as();
}

void MenuCommandRegistrar::save_project_as()
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
                            AppServices::instance().app_config().get<std::string>(
                                "last_used_directory"
                            )
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

// Maps UI language to supported website locale codes.
static std::string current_language_code_safe()
{
    // Translate the language code to a code, for which Prusa Research maintains translations.
    const std::map<std::string, std::string> mapping{
        {
            "cs",
            "cs_CZ",
        },
        {
            "sk",
            "cs_CZ",
        },
        {
            "de",
            "de_DE",
        },
        {
            "es",
            "es_ES",
        },
        {
            "fr",
            "fr_FR",
        },
        {
            "it",
            "it_IT",
        },
        {
            "ja",
            "ja_JP",
        },
        {
            "ko",
            "ko_KR",
        },
        {
            "pl",
            "pl_PL",
        },
        //{ "uk", 	"uk_UA", },
        //{ "zh", 	"zh_CN", },
        //{ "ru", 	"ru_RU", },
    };

    std::string language_code = localization().active_language();
    auto it                   = mapping.find(language_code);
    if (it != mapping.end())
        language_code = it->second;
    else
        language_code = "en_US";
    return language_code;
}

void MenuCommandRegistrar::open_browser(OpenBrowserParams params)
{
    if (params.is_localized_url) {
        params.url += "&lng=" + current_language_code_safe();
    }

    enum class SuppressHyperLinksOption
    {
        ShowWarning,
        AlwaysSuppress,
        AlwaysAllow
    };

    AppConfig& app_config          = AppServices::instance().app_config();
    IDialogManager& dialog_manager = AppServices::instance().dialog_manager();
    bool show_warning              = app_config.get<bool>("show_open_browser_warning_dialog");
    bool checked                   = app_config.get<bool>("suppress_hyperlinks");
    SuppressHyperLinksOption opt_val =
        (show_warning ? SuppressHyperLinksOption::ShowWarning :
                        (checked ? SuppressHyperLinksOption::AlwaysSuppress :
                                   SuppressHyperLinksOption::AlwaysAllow));
    bool launch = true;
    if (opt_val == SuppressHyperLinksOption::ShowWarning) {
        // no previous action from user
        // open dialog with remember checkbox
        dialog_manager.show_rich_yesno_dialog(
            _u8L("PrusaSlicer: Open hyperlink"),
            _u8L("Open hyperlink in default browser?"),
            _u8L("Remember my choice"),
            [&launch](bool answer) { launch = answer; },
            [&launch](bool checked)
            {
                if (checked) {
                    // ysFIXME: use AppConfigInteractor , when it will be merged
                    AppServices::instance().app_config_interactor().set_item_value(
                        "show_open_browser_warning_dialog",
                        Domain::ConfigValue(false)
                    );
                    AppServices::instance().app_config_interactor().set_item_value(
                        "suppress_hyperlinks",
                        Domain::ConfigValue(!launch)
                    );
                }
            }
        );
    } else if (opt_val == SuppressHyperLinksOption::AlwaysAllow) {
        // user already set checkbox to always open
        launch = true;
    } else if (opt_val == SuppressHyperLinksOption::AlwaysSuppress && params.force_remember_choice)
    {
        // user already set checkbox or preferences to always supress
        launch = false;
    } else if (opt_val == SuppressHyperLinksOption::AlwaysSuppress && !params.force_remember_choice)
    {
        // user already set checkbox or preferences to always supress but it is overriden
        // no checkbox in dialog
        dialog_manager.show_yesno_dialog(
            _u8L("PrusaSlicer: Open hyperlink"),
            _u8L("Open hyperlink in default browser?"),
            [&](bool answer) { launch = answer; }
        );
    }

    if (launch) {
        dialog_manager.open_in_browser(params.url, 0);
    }
}

void MenuCommandRegistrar::register_main_menu_edit_commands()
{
    m_menu_manager
        // Menu -> Edit -> Select All
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::SelectAll},
            std::make_unique<UIItemCommand>(
                CommandName::SelectAll,
                [this]()
                {
                    Biz::Scene::ObjectSelection selection{Biz::Scene::SelectionMode::Instance, {}};

                    const Domain::Model& model = m_project_interactor.selected_project().model();
                    for (const auto object : model.objects) {
                        for (const auto instance : object->instances) {
                            selection.elements.push_back({object->id().id, instance->id().id});
                        }
                    }
                    m_project_interactor.scene_interactor().set_object_selection(selection);
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::A
                        }
                }
            )
        )
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
        // Menu -> Edit -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu, MenuItemName::Edit})
        // Menu -> Edit -> Delete Selected
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
#ifdef SHOW_NOT_IMPLEMENTED_ITEMS
        // Menu -> Edit -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu, MenuItemName::Edit})
        // Menu -> Edit -> Undo
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::Undo},
            std::make_unique<UIItemCommand>(
                CommandName::Undo,
                [this]()
                {
                    // TODO: Implement undo functionality
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::Z
                        }
                }
            )
        )
        // Menu -> Edit -> Redo
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::Redo},
            std::make_unique<UIItemCommand>(
                CommandName::Redo,
                [this]()
                {
                    // TODO: Implement redo functionality
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::Y
                        }
                }
            )
        )
        // Menu -> Edit -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu, MenuItemName::Edit})
        // Menu -> Edit -> Copy
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::Copy},
            std::make_unique<UIItemCommand>(
                CommandName::Copy,
                [this]()
                {
                    // TODO: Implement copy functionality
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::C
                        }
                }
            )
        )
        // Menu -> Edit -> Paste
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::Paste},
            std::make_unique<UIItemCommand>(
                CommandName::Paste,
                [this]()
                {
                    // TODO: Implement paste functionality
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::V
                        }
                }
            )
        )
        // Menu -> Edit -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu, MenuItemName::Edit})
        // Menu -> Edit -> Reload From Disk
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::ReloadFromDisk},
            std::make_unique<UIItemCommand>(
                CommandName::ReloadFromDisk,
                [this]()
                {
                    // TODO: Implement reload from disk functionality
                }
            )
        )
#endif // SHOW_NOT_IMPLEMENTED_ITEMS
        // Menu -> Edit -> Search
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Edit, MenuItemName::Search},
            std::make_unique<UIItemCommand>(
                CommandName::Search,
                [this]() { m_navigator.request_search(); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut = Platform::KeyboardShortcut{
                        Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                        Platform::KeyCode::F
                    }
                }
            )
        );
}

void MenuCommandRegistrar::register_main_menu_view_commands()
{
    m_menu_manager
#ifdef SHOW_NOT_IMPLEMENTED_ITEMS
        // Menu -> View -> Show Label
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::View, MenuItemName::ShowLabel},
            std::make_unique<UIItemCommand>(
                CommandName::ShowLabel,
                [this]()
                {
                    // TODO: Implement show label functionality
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut = Platform::KeyboardShortcut{0, Platform::KeyCode::E}
                }
            )
        )
#endif
        // Menu -> View -> Change Camera Type
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::View, MenuItemName::ChangeCameraType},
            std::make_unique<UIItemCommand>(
                CommandName::ChangeCameraType,
                [this]()
                {
                    // TODO: Implement change camera type functionality
                }
            )
        );
#ifndef __APPLE__
    // OSX adds its own menu item to toggle fullscreen.
    if (m_navigator.has_fullscreen()) {
        // Menu -> View -> Separator
        m_menu_manager.register_menu_separator_item({ MenuItemName::MainMenu, MenuItemName::View })
        // Menu -> View -> Full Screen
        .register_menu_item(
            { MenuItemName::MainMenu, MenuItemName::View, MenuItemName::FullScreen },
            std::make_unique<UIItemCommand>(
                CommandName::FullScreen,
                [this]()
                {
                    m_navigator.set_fullscreen(!m_navigator.is_fullscreen());
                },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut = Platform::KeyboardShortcut{0, Platform::KeyCode::F11},
                    .checked = [this]() {return m_navigator.is_fullscreen(); }
                }
            )
        );
    }
#endif // __APPLE__
}

void MenuCommandRegistrar::register_main_menu_config_commands()
{
#ifdef SHOW_NOT_IMPLEMENTED_ITEMS
    m_menu_manager
        // Menu -> Configuration -> Configuration Wizard
        .register_menu_item(
            {MenuItemName::MainMenu,
             MenuItemName::Configuration,
             MenuItemName::ConfigurationWizard},
            std::make_unique<UIItemCommand>(
                CommandName::ConfigurationWizard,
                [this]()
                {
                    // TODO: Implement configuration wizard functionality
                }
            )
        )
        // Menu -> Configuration -> Check Config Updates
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Configuration, MenuItemName::CheckConfigUpdates},
            std::make_unique<UIItemCommand>(
                CommandName::CheckConfigUpdates,
                [this]()
                {
                    // TODO: Implement check config updates functionality
                }
            )
        )
        // Menu -> Configuration -> Check App Updates
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Configuration, MenuItemName::CheckAppUpdates},
            std::make_unique<UIItemCommand>(
                CommandName::CheckAppUpdates,
                [this]()
                {
                    // TODO: Implement check app updates functionality
                }
            )
        )
        // Menu -> Configuration -> Flash Printer Firmware
        .register_menu_item(
            {MenuItemName::MainMenu,
             MenuItemName::Configuration,
             MenuItemName::FlashPrinterFirmware},
            std::make_unique<UIItemCommand>(
                CommandName::FlashPrinterFirmware,
                [this]()
                {
                    // TODO: Implement flash printer firmware functionality
                }
            )
        );
#endif
}

void MenuCommandRegistrar::register_main_menu_help_commands()
{
    m_menu_manager
        // Menu -> Help -> Prusa Website
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::PSWebsite},
            std::make_unique<UIItemCommand>(
                CommandName::PSWebsite,
                [this]()
                {
                    open_browser(
                        OpenBrowserParams{
                            .url              = "https://www.prusa3d.com/slicerweb",
                            .is_localized_url = true
                        }
                    );
                }
            )
        )
        // Menu -> Help -> Quick Start
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::QuickStart},
            std::make_unique<UIItemCommand>(
                CommandName::QuickStart,
                [this]()
                {
                    open_browser(
                        OpenBrowserParams{
                            .url =
                                "https://help.prusa3d.com/article/first-print-with-prusaslicer_1753",
                            .force_remember_choice = false
                        }
                    );
                }
            )
        )
        // Menu -> Help -> Samples
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::Samples},
            std::make_unique<UIItemCommand>(
                CommandName::Samples,
                [this]()
                {
                    open_browser(
                        OpenBrowserParams{
                            .url = "https://help.prusa3d.com/article/sample-g-codes_529630",
                            .force_remember_choice = false
                        }
                    );
                }
            )
        )
        // Menu -> Help -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu, MenuItemName::Help})
        // Menu -> Help -> Prusa 3D Drivers
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::Prusa3DDrivers},
            std::make_unique<UIItemCommand>(
                CommandName::Prusa3DDrivers,
                [this]()
                {
                    open_browser(
                        OpenBrowserParams{
                            .url              = "https://www.prusa3d.com/downloads",
                            .is_localized_url = true
                        }
                    );
                }
            )
        )
        // Menu -> Help -> Releases
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::Releases},
            std::make_unique<UIItemCommand>(
                CommandName::Releases,
                [this]()
                {
                    open_browser(
                        OpenBrowserParams{
                            .url = "https://github.com/prusa3d/PrusaSlicer/releases",
                            .force_remember_choice = false
                        }
                    );
                }
            )
        )
        // Menu -> Help -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu, MenuItemName::Help})
#ifdef SHOW_NOT_IMPLEMENTED_ITEMS
        // Menu -> Help -> System Info
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::SystemInfo},
            std::make_unique<UIItemCommand>(
                CommandName::SystemInfo,
                [this]()
                {
                    // TODO: Implement system info functionality
                }
            )
        )
        // Menu -> Help -> Config Folder
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::ConfigFolder},
            std::make_unique<UIItemCommand>(
                CommandName::ConfigFolder,
                [this]()
                {
                    // TODO: Implement config folder functionality
                }
            )
        )
#endif
        // Menu -> Help -> Report An Issue
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::ReportAnIssue},
            std::make_unique<UIItemCommand>(
                CommandName::ReportAnIssue,
                [this]()
                {
                    open_browser(
                        OpenBrowserParams{
                            .url                   = "https://github.com/prusa3d/slic3r/issues/new",
                            .force_remember_choice = false
                        }
                    );
                }
            )
        )
#ifdef SHOW_NOT_IMPLEMENTED_ITEMS
        // Menu -> Help -> About
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::About},
            std::make_unique<UIItemCommand>(
                CommandName::About,
                [this]()
                {
                    // TODO: Implement about functionality
                }
            )
        )
        // Menu -> Help -> Tip Of The Day
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::TipOfTheDay},
            std::make_unique<UIItemCommand>(
                CommandName::TipOfTheDay,
                [this]()
                {
                    // TODO: Implement tip of the day functionality
                }
            )
        )
        // Menu -> Help -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu, MenuItemName::Help})
        // Menu -> Help -> Keyboard Shortcuts
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Help, MenuItemName::KeyboardShortcuts},
            std::make_unique<UIItemCommand>(
                CommandName::KeyboardShortcuts,
                [this]()
                {
                    // TODO: Implement keyboard shortcuts functionality
                }
            )
        );
#endif
}

void MenuCommandRegistrar::register_main_menu_commands()
{
    register_main_menu_edit_commands();

#ifdef SHOW_NOT_IMPLEMENTED_ITEMS
    m_menu_manager
        // Menu -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu})
        // Menu -> Shape Gallery
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::ShapeGallery},
            std::make_unique<UIItemCommand>(
                CommandName::ShapeGallery,
                [this]()
                {
                    // TODO: Implement shape gallery functionality
                }
            )
        );
#endif
    register_main_menu_view_commands();

    m_menu_manager
        // Menu -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu})
        // Menu -> Preferences
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Preferences},
            std::make_unique<UIItemCommand>(
                CommandName::Preferences,
                [this]() { m_navigator.set_opened_preferences(true); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut = Platform::KeyboardShortcut{
                        Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                        Platform::KeyCode::P
                    }
                }
            )
        );

    register_main_menu_config_commands();

    m_menu_manager
        // Menu -> Separator
        .register_menu_separator_item({MenuItemName::MainMenu});

    register_main_menu_help_commands();

    m_menu_manager
        // Menu -> Exit
        .register_menu_item(
            {MenuItemName::MainMenu, MenuItemName::Exit},
            std::make_unique<UIItemCommand>(
                "",
                [this]()
                {
                    m_navigator.close_application();
                }
            )
        );
}

void MenuCommandRegistrar::register_file_menu_import_commands()
{
    m_menu_manager
        // File -> Import -> Import Geometry
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
                            AppServices::instance().app_config().get<std::string>(
                                "last_used_directory"
                            )
                        ),
                        "",
                        Wildcards::generate_wildcards(
                            Wildcards::TypeFlag::Project3mf
                                | Wildcards::TypeFlag::Stl
                                | Wildcards::TypeFlag::Obj
                                | Wildcards::TypeFlag::Step,
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

void MenuCommandRegistrar::register_file_menu_export_commands()
{
    m_menu_manager
        // File -> Export -> Export G-code
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::Export, MenuItemName::ExportGcode},
            std::make_unique<UIItemCommand>(
                CommandName::ExportGcode,
                ExportActions::export_gcode(m_project_interactor),
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::G
                        },
                    .enabled = [this]() { return ExportActions::can_export(m_project_interactor); }
                }
            )
        )
        // File -> Export -> Send G-code
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::Export, MenuItemName::SendGcode},
            std::make_unique<UIItemCommand>(
                CommandName::SendGcode,
                ExportActions::send_gcode_to_connect(m_project_interactor),
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl)
                                | Platform::KeyModifiers(Platform::KeyModifier::Shift),
                            Platform::KeyCode::G
                        },
                    .enabled =
                        [this]()
                    {
                        return ExportActions::can_export(m_project_interactor)
                            && m_project_interactor.user_account_interactor().is_logged_in();
                    }
                }
            )
        )
        // File -> Export -> Export G-code to SD Card / Flash Drive
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::Export, MenuItemName::ExportGcodeToFlash},
            std::make_unique<UIItemCommand>(
                CommandName::ExportGcodeToFlash,
                ExportActions::export_gcode_to_flash(m_project_interactor),
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::U
                        },
                    .enabled = [this]()
                    {
                        return ExportActions::can_export(m_project_interactor)
                            && m_project_interactor.removable_drive_service()
                                   .has_removable_drives();
                    }
                }
            )
        );
}

void MenuCommandRegistrar::register_file_menu_commands()
{
    m_menu_manager
        // File -> New Project
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
        // File -> Open Project
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
        // File -> Save Project
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::SaveProject},
            std::make_unique<UIItemCommand>(
                CommandName::SaveProject,
                [this]() { save_project(); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl),
                            Platform::KeyCode::S
                        }
                }
            )
        )
        // File -> Save Project As
        .register_menu_item(
            {MenuItemName::FileMenu, MenuItemName::SaveProjectAs},
            std::make_unique<UIItemCommand>(
                CommandName::SaveProjectAs,
                [this]() { save_project_as(); },
                UIItemCommandExtraOpts{
                    .keyboard_shortcut =
                        Platform::KeyboardShortcut{
                            Platform::KeyModifiers(Platform::KeyModifier::Ctrl)
                                | Platform::KeyModifiers(Platform::KeyModifier::Shift),
                            Platform::KeyCode::S
                        }
                }
            )
        )
        // File -> Separator
        .register_menu_separator_item({MenuItemName::FileMenu});

    register_file_menu_import_commands();
    register_file_menu_export_commands();
}

} // namespace Slic3r::App
