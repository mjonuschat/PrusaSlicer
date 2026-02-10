#pragma once

#include <string>

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class MenuManager;
struct ThumbnailStore;
class Navigator;

class MenuCommandRegistrar
{
public:
    MenuCommandRegistrar(
        MenuManager& menu_manager,
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator,
        ThumbnailStore& thumbnail_store
    );

private:
    void register_all();

    void register_main_menu_commands();
    void register_main_menu_edit_commands();
    void register_main_menu_view_commands();
    void register_main_menu_config_commands();
    void register_main_menu_help_commands();

    void register_file_menu_commands();
    void register_file_menu_import_commands();
    void register_file_menu_export_commands();

    void load_project();
    void save_project();
    void save_project_as();

    struct OpenBrowserParams
    {
        std::string url;
        bool force_remember_choice{true};
        bool is_localized_url{false};
    };
    void open_browser(OpenBrowserParams params);

    MenuManager& m_menu_manager;
    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;
    ThumbnailStore& m_thumbnail_store;
};

} // namespace Slic3r::App
