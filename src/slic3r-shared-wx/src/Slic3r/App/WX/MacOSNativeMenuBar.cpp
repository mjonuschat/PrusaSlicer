///|/ Copyright (c) Prusa Research 2024
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/WX/MacOSNativeMenuBar.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"

#ifdef USE_NATIVE_MENU

#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/MenuItem.hpp"
#include "Slic3r/App/MenuBuilder.hpp"
#include "Slic3r/App/UIItemCommand.hpp"
#include "Slic3r/App/CommandBindingManager.hpp"
#include "Slic3r/App/Platform/CommandName.hpp"
#include "Slic3r/App/Platform/ICommand.hpp"
#include "Slic3r/App/Platform/KeyboardShortcut.hpp"

namespace Slic3r::App::WX {

MacOSNativeMenuBar::MacOSNativeMenuBar(
    MenuManager& menu_manager,
    CommandBindingManager& command_binding_manager,
    std::function<void()> force_focused_canvas
) :
    m_menu_manager(menu_manager),
    m_command_binding_manager(command_binding_manager),
    m_force_focused_canvas(force_focused_canvas)
{}

MacOSNativeMenuBar::~MacOSNativeMenuBar()
{
    // wxMenuBar is owned by wxFrame after SetMenuBar(), so we don't delete it
}

void MacOSNativeMenuBar::build_from_menu_manager()
{
    m_menu_bar = new wxMenuBar();

    // Build FileMenu
    build_menu_from_name(MenuItemName::FileMenu);

    // Build Edit menu (Edit button in TopBar)
    build_menu_from_name(MenuItemName::Edit);

    // Build View menu (View button in TopBar)
    build_menu_from_name(MenuItemName::View);

    // Build Config Menu
    build_menu_from_name(MenuItemName::Configuration);

    // Build Help menu (Help button in TopBar)
    build_menu_from_name(MenuItemName::Help);
}

void MacOSNativeMenuBar::build_menu_from_name(MenuItemName menu_item_name)
{
    if (MenuItem* menu = m_menu_manager.menu_item(menu_item_name)) {
        wxMenu* wx_menu = build_menu_from_item(menu);
        if (wx_menu) {
            m_menu_bar->Append(wx_menu, from_u8(MenuBuilder::item_name_translated(menu_item_name)));
        }
    }
}

wxMenu* MacOSNativeMenuBar::build_menu_from_item(MenuItem* menu_item)
{
    if (!menu_item || menu_item->children().empty()) {
        return nullptr;
    }

    wxMenu* wx_menu = new wxMenu();
    populate_menu(wx_menu, menu_item);
    return wx_menu;
}

void MacOSNativeMenuBar::add_menu_item_to_menu(
    wxMenu* wx_menu,
    MenuItem* item,
    int id /* = wxID_ANY*/,
    int pos /*= wxNOT_FOUND*/
)
{
    const UIItemCommand* cmd = item->command();
    if (!cmd) {
        return;
    }

    wxString label = from_u8(MenuBuilder::item_name_translated(item->name()));

    // Add keyboard shortcut to label if available
    wxString shortcut = get_shortcut_string(cmd->name());
    if (!shortcut.empty()) {
        label += from_u8("\t") + shortcut;
    }

    if (id == wxID_ANY) {
        id = wxNewId();
    }
    wxMenuItem* wx_item =
        pos == wxNOT_FOUND ? wx_menu->Append(id, label) : wx_menu->Insert(size_t(pos), id, label);

    if (std::string icon_name = MenuBuilder::icon_name(item->name()); !icon_name.empty()) {
        wx_item->SetBitmap(*get_bmp_bundle_for_mac_menu(icon_name));
    }

    // Store mapping from ID to command name
    m_id_to_command[id] = cmd->name();

    // Bind menu event to execute command
    wx_menu->Bind(
        wxEVT_MENU,
        [this, cmd_name = std::string(cmd->name())](wxCommandEvent& e)
        { process_command_event(cmd_name.c_str()); },
        id
    );

    // Set initial enabled state
    if (!cmd->enabled()) {
        wx_item->Enable(false);
    }
}

void MacOSNativeMenuBar::populate_menu(wxMenu* wx_menu, MenuItem* parent_item)
{
    for (MenuItem* item : parent_item->children()) {
        if (item->is_separator()) {
            wx_menu->AppendSeparator();
            continue;
        }

        wxString label = from_u8(MenuBuilder::item_name_translated(item->name()));

        if (!item->children().empty()) {
            // This is a submenu
            wxMenu* submenu = new wxMenu();
            populate_menu(submenu, item);
            wx_menu->AppendSubMenu(submenu, label);
        } else {
            // This is a regular menu item with a command
            add_menu_item_to_menu(wx_menu, item);
        }
    }
}

wxString MacOSNativeMenuBar::get_shortcut_string(const char* command_name)
{
    if (!command_name
        || command_name[0] == '\0'
        || !m_command_binding_manager.has_command(command_name))
    {
        return wxString();
    }

    const Platform::ICommand& cmd = m_command_binding_manager.command(command_name);
    if (!cmd.keyboard_shortcuts().has_value()) {
        return wxString();
    }

    // We check just first shortcut from the list
    auto shortcut = cmd.keyboard_shortcuts().value().front();

#ifdef __APPLE__
    // Skip unmodified letter and numbers shortcuts — these must reach the canvas
    // as raw key events, not be intercepted by Cocoa key equivalents.
    if (shortcut.modifiers == 0
        && ((shortcut.key >= Platform::KeyCode::A && shortcut.key <= Platform::KeyCode::Z)
            || (shortcut.key >= Platform::KeyCode::Num0
                && shortcut.key <= Platform::KeyCode::Num9)))
    {
        return wxString();
    }
#endif

    // Use the platform-specific accelerator table string
    std::string accel_str = shortcut.to_accel_table_string();
    return from_u8(accel_str);
}

void MacOSNativeMenuBar::process_command_event(const char* command_name)
{
    ASSERT(
        m_command_binding_manager.has_command(command_name),
        fmt::format("Try to process non-bind command \"{}\"", command_name)
    );
    const Platform::ICommand& cmd = m_command_binding_manager.command(command_name);
    if (m_force_focused_canvas) {
        m_force_focused_canvas();
    }
    if (cmd.enabled()) {
        cmd.execute();
    }
}

void MacOSNativeMenuBar::setup_apple_menu()
{
    if (!m_menu_bar) {
        return;
    }

#ifdef __APPLE__
    wxMenu* apple_menu = m_menu_bar->OSXGetAppleMenu();
#else
    wxMenu* apple_menu = new wxMenu();
    if (apple_menu)
        m_menu_bar->Append(
            apple_menu,
            from_u8(MenuBuilder::item_name_translated(MenuItemName::MainMenu))
        );
#endif
    if (!apple_menu) {
        return;
    }

    // Add About item at the top
    // Note: wxID_ABOUT is automatically placed in the Apple menu on macOS
    add_menu_item_to_menu(apple_menu, m_menu_manager.menu_item(MenuItemName::About), wxID_ABOUT, 0);

    // Preferences is automatically handled by wxWidgets on macOS (Cmd+,)
    // It uses wxID_PREFERENCES
    add_menu_item_to_menu(
        apple_menu,
        m_menu_manager.menu_item(MenuItemName::Preferences),
        wxID_PREFERENCES,
        1
    );

    // Quit handler (Cmd+Q)
    apple_menu->Bind(
        wxEVT_MENU,
        [this](wxCommandEvent&) { process_command_event(Platform::CommandName::Exit); },
        wxID_EXIT
    );
}

void MacOSNativeMenuBar::update_menu_states()
{
    if (!m_menu_bar) {
        return;
    }

    for (const auto& [id, command_name] : m_id_to_command) {
        ASSERT(
            m_command_binding_manager.has_command(command_name.c_str()),
            fmt::format("Try to process non-bind command \"{}\"", command_name)
        );
        const Platform::ICommand& cmd = m_command_binding_manager.command(command_name.c_str());
        wxMenuItem* item              = m_menu_bar->FindItem(id);
        if (item) {
            item->Enable(cmd.enabled());
        }
    }
}

void MacOSNativeMenuBar::on_user_account_id_success(bool, const std::string&)
{
    update_menu_states();
}

void MacOSNativeMenuBar::on_user_account_logged_out()
{
    update_menu_states();
}

void MacOSNativeMenuBar::on_selected_bed_instances_changed(
    Domain::SelectionId,
    const Biz::Scene::BedSelection&
)
{
    update_menu_states();
}

void MacOSNativeMenuBar::on_status_cache_status_code_changed(const Domain::SlicingId)
{
    update_menu_states();
}

void MacOSNativeMenuBar::on_removable_drive_status_changed(
    const boost::filesystem::path&,
    Biz::RemovableDrive::RemovableDriveStatus
)
{
    update_menu_states();
}

} // namespace Slic3r::App::WX

#endif // USE_NATIVE_MENU
