#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/MenuItem.hpp"
#include "Slic3r/App/UIItemCommand.hpp"

#include "Slic3r/App/Platform/CommandRegistry.hpp"

#include "Slic3r/Assert.hpp"

namespace Slic3r::App {

MenuManager& MenuManager::register_menu_item(
    std::vector<MenuItemName> path,
    std::unique_ptr<UIItemCommand> command
)
{
    UIItemCommand* cmd = command.get();
    m_command_registry.register_command(std::move(command));

    // Add new menu item

    MenuItemName child_name = path.back();
    ASSERT(!m_menus_by_id.contains(child_name));
    m_menus_by_id[child_name] = std::make_unique<MenuItem>(child_name, cmd);
    path.pop_back();

    if (path.empty())
        return *this;

    // Create parent if it isn't exist jet

    MenuItemName parent_name = path.back();
    if (!m_menus_by_id.contains(parent_name)) {
        m_menus_by_id[parent_name] = std::make_unique<MenuItem>(parent_name);
    }

    // Append child for the parent
    // Note: All parents doesn't have command but have all other needed info

    m_menus_by_id[parent_name]->append(m_menus_by_id[child_name].get());
    path.pop_back();

    // Distribute it to the upper parents if needed

    while (!path.empty()) {
        child_name  = parent_name;
        parent_name = path.back();
        if (!m_menus_by_id.contains(parent_name)) {
            m_menus_by_id[parent_name] = std::make_unique<MenuItem>(parent_name);
            m_menus_by_id[parent_name]->append(m_menus_by_id[child_name].get());
        }
        path.pop_back();
    }

    return *this;
}

MenuItem* MenuManager::menu_item(MenuItemName name)
{
    if (m_menus_by_id.count(name) == 0) {
        return nullptr;
    }
    ASSERT(m_menus_by_id.contains(name), "Non-existed command");
    return m_menus_by_id[name].get();
}
} // namespace Slic3r::App::MenuManager
