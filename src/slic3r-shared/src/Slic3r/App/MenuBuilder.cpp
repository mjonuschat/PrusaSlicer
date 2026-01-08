#include "Slic3r/App/MenuBuilder.hpp"
#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/MenuItem.hpp"
#include "Slic3r/App/CommandBindingManager.hpp"
#include "Slic3r/App/UIItemCommand.hpp"

#include "Slic3r/App/Yoga/Menu.hpp"
#include "Slic3r/App/Yoga/MenuItem.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App {

std::string dots{"..."};

std::string MenuBuilder::item_name_translated(MenuItemName menu_item_name)
{
    switch (menu_item_name) {
    case MenuItemName::MainMenu:
        return Biz::_u8L("Menu");
    case MenuItemName::Edit:
        return Biz::_u8L("Edit");
    case MenuItemName::DeselectAll:
        return Biz::_u8L("Deselect All");
    case MenuItemName::DeleteSelected:
        return Biz::_u8L("Delete selected");
    case MenuItemName::Search:
        return Biz::_u8L("Search");
    case MenuItemName::Preferences:
        return Biz::_u8L("Preferences");
    case MenuItemName::FileMenu:
        return Biz::_u8L("File");
    case MenuItemName::NewProject:
        return Biz::_u8L("New project");
    case MenuItemName::OpenProject:
        return Biz::_u8L("Open project");
    case MenuItemName::SaveProject:
        return Biz::_u8L("Save");
    case MenuItemName::SaveProjectAs:
        return Biz::_u8L("Save as") + dots;
    case MenuItemName::Import:
        return Biz::_u8L("Import");
    case MenuItemName::ImportGeometry:
        return Biz::_u8L("Import STL/3MF");
    case MenuItemName::JumpToValue:
        return Biz::_u8L("Jump to height");
    default:
        return std::string();
    }
};

Render::Icon MenuBuilder::item_icon(MenuItemName menu_item_name)
{
    switch (menu_item_name) {
    case MenuItemName::MainMenu:
        return Render::Icon::PrusaSlicerIcon;
    case MenuItemName::DeleteSelected:
        return Render::Icon::DeleteBtnIcon;
    case MenuItemName::Search:
        return Render::Icon::Search;
    case MenuItemName::Preferences:
        return Render::Icon::Cog;
    case MenuItemName::FileMenu:
    case MenuItemName::NewProject:
        return Render::Icon::NewBtnIcon;
    case MenuItemName::OpenProject:
        return Render::Icon::TobBarLoad;
    case MenuItemName::SaveProject:
    case MenuItemName::SaveProjectAs:
        return Render::Icon::TobBarSave;
    case MenuItemName::ImportGeometry:
        return Render::Icon::CubeAdd;

    case MenuItemName::Edit:
    case MenuItemName::Import:
    case MenuItemName::DeselectAll:
    case MenuItemName::JumpToValue:
    default:
        return Render::Icon::None;
    }
}

void MenuBuilder::add_submenu(Yoga::MenuItem* yoga_menu_item, App::MenuItem* menu_item)
{
    for (App::MenuItem* sub_menu_item : menu_item->children()) {
        Yoga::MenuItem* new_yoga_menu_item = yoga_menu_item->append_sub_menu_item(
            item_name_translated(sub_menu_item->name()),
            nullptr,
            item_icon(sub_menu_item->name()),
            sub_menu_item->command()->keyboard_shortcut_string()
        );
        if (sub_menu_item->children().empty()) {
            m_command_binding_manager.bind_menu_item(sub_menu_item->command(), new_yoga_menu_item);
        } else {
            add_submenu(new_yoga_menu_item, sub_menu_item);
        }
    }
}

Yoga::MenuItem* MenuBuilder::add_menu_item(Yoga::Menu* menu, App::MenuItem* menu_item)
{
    Yoga::MenuItem* yoga_menu_item = menu->append_item(
        item_name_translated(menu_item->name()),
        nullptr,
        item_icon(menu_item->name()),
        menu_item->command()->keyboard_shortcut_string()
    );
    m_command_binding_manager.bind_menu_item(menu_item->command(), yoga_menu_item);

    return yoga_menu_item;
}

void MenuBuilder::add_menu_items(Yoga::Menu* menu, App::MenuItem* root_menu_item)
{
    ASSERT(root_menu_item);

    for (App::MenuItem* menu_item : root_menu_item->children()) {
        if (menu_item->children().empty()) {
            Yoga::MenuItem* yoga_menu_item = add_menu_item(menu, menu_item);
        } else {
            Yoga::MenuItem* yoga_menu_item_with_submenu =
                menu->append_item_as_menu(item_name_translated(menu_item->name()));
            add_submenu(yoga_menu_item_with_submenu, menu_item);
        }
    }
}

Yoga::MenuItem* MenuBuilder::add_menu_item(Yoga::Menu* menu, MenuItemName menu_item_name)
{
    return add_menu_item(menu, m_menu_manager.menu_item(menu_item_name));
}
} // namespace Slic3r::App
