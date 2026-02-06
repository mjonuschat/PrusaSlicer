///|/ Copyright (c) Prusa Research 2024
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Platform/AbstractRenderCanvas.hpp"

#ifdef USE_NATIVE_MENU

#include <wx/menu.h>
#include <functional>
#include <unordered_map>

#include "Slic3r/Biz/ProjectScoped.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"

namespace Slic3r::App {
class MenuManager;
class MenuItem;
class CommandBindingManager;
enum class MenuItemName;
} // namespace Slic3r::App

namespace Slic3r::App::WX {

/**
 * @brief Builds and manages native macOS menu bar from MenuManager structure.
 *
 * This class creates a wxMenuBar that mirrors the menu structure registered
 * in MenuManager, allowing the native macOS system menu bar to work alongside
 * custom ImGui menus.
 *
 * Usage:
 * 1. Create instance after MenuManager is populated with menu items
 * 2. Call build_from_menu_manager() to create the wxMenuBar
 * 3. Call get_menu_bar() to retrieve and attach to wxFrame
 * 4. Call setup_apple_menu() after SetMenuBar() to configure About/Quit
 */
class MacOSNativeMenuBar :
    public Biz::UserAccount::IUserAccountListener,
    public Biz::IStatusCacheChangedListener,
    public Biz::ISelectedBedInstancesChangedListener,
    public Biz::RemovableDrive::IRemovableDriveStatusListener
{
public:
    /**
     * @brief Construct native menu bar builder
     * @param menu_manager Source of menu structure
     * @param command_binding_manager Binding manager to execute commands when menu items are clicked
     */
    MacOSNativeMenuBar(
        MenuManager& menu_manager,
        CommandBindingManager& command_binding_manager,
        std::function<void()> force_focused_canvas
    );

    ~MacOSNativeMenuBar();

    /**
     * @brief Build the wxMenuBar from MenuManager structure
     *
     * Creates wxMenu instances for MainMenu and FileMenu, populating them
     * with items registered in MenuManager. Each menu item is connected
     * to its corresponding command in Binding manager.
     */
    void build_from_menu_manager();

    /**
     * @brief Get the built wxMenuBar
     * @return Pointer to wxMenuBar, or nullptr if build_from_menu_manager() not called
     */
    wxMenuBar* get_menu_bar() const
    {
        return m_menu_bar;
    }

    /**
     * @brief Setup macOS Apple menu (About, Preferences, Quit)
     *
     * Must be called AFTER wxFrame::SetMenuBar() to access OSXGetAppleMenu().
     * Configures standard macOS application menu items.
     */
    void setup_apple_menu();

    /**
     * @brief Update menu item enabled states
     *
     * Call periodically or on state changes to sync enabled/disabled
     * states with Binding manager.
     */
    void update_menu_states();

    void on_user_account_id_success(bool, const std::string&) override;
    void on_user_account_logged_out() override;

    void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::BedSelection& bed_selection
    ) override;
    void on_status_cache_status_code_changed(const Domain::SlicingId slicing_id) override;

    void on_removable_drive_status_changed(
        const boost::filesystem::path&,
        Biz::RemovableDrive::RemovableDriveStatus
    ) override;

private:
    void build_menu_from_name(MenuItemName menu_item_name);
    wxMenu* build_menu_from_item(MenuItem* menu_item);
    void populate_menu(wxMenu* wx_menu, MenuItem* parent_item);
    void add_menu_item_to_menu(
        wxMenu* wx_menu,
        MenuItem* item,
        int item_id = wxID_ANY,
        int pos     = wxNOT_FOUND
    );
    wxString get_shortcut_string(const char* command_name);

    void process_command_event(const char* command_name);

    MenuManager& m_menu_manager;
    CommandBindingManager& m_command_binding_manager;
    std::function<void()> m_force_focused_canvas{nullptr};
    wxMenuBar* m_menu_bar{nullptr};

    // Maps wxMenuItem IDs to command names for execution
    std::unordered_map<int, std::string> m_id_to_command;
};

} // namespace Slic3r::App::Platform::WX

#endif // USE_NATIVE_MENU
