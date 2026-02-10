#pragma once

#include "Slic3r/App/MenuItemName.hpp"
#include "Slic3r/App/MenuItem.hpp"

#include <unordered_map>
#include <memory>

namespace Slic3r::App {

namespace Platform {
class CommandRegistry;
} // namespace Platform

class UIItemCommand;
class CommandBindingManager;

class MenuManager
{
public:
    using MenusMap = std::unordered_map<MenuItemName, std::unique_ptr<MenuItem>>;

    MenuManager(Platform::CommandRegistry& command_registry) : m_command_registry(command_registry)
    {}

    /**
     * @brief Register single menu item with related command
     * @param path Path to menu item as a stack of MenuItemNames, where first is a root item name
     * @param command Pointer to command implementation
     */
    virtual MenuManager&
    register_menu_item(std::vector<MenuItemName> path, std::unique_ptr<UIItemCommand> command);

    /**
     * @brief Register menu separator item
     * @param path Path to parent menu as a stack of MenuItemNames, where first is a root item name
     */
    virtual MenuManager&
    register_menu_separator_item(std::vector<MenuItemName> path);

    MenuItem* menu_item(MenuItemName name);

private:
    void distribute_into_menu_hierarchy(MenuItemName child_name, std::vector<MenuItemName> path);

private:
    Platform::CommandRegistry& m_command_registry;
    MenusMap m_menus_by_id;
};

} // namespace Slic3r::App
