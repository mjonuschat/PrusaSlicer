#pragma once

#include <string>
#include <vector>

#include "Slic3r/App/MenuItemName.hpp"
#include "Slic3r/App/UIItemCommand.hpp"
#include <Slic3r/Assert.hpp>

namespace Slic3r::App {

class MenuItem
{
public:
    MenuItem(MenuItemName name, const UIItemCommand* command = nullptr) :
        m_name(name),
        m_command(command) {};

    void append(MenuItem* child)
    {
        ASSERT(!is_separator());
        m_children.emplace_back(child);
    }

    const MenuItemName name() const
    {
        return m_name;
    }

    const UIItemCommand* command() const
    {
        return m_command;
    }

    const std::vector<MenuItem*>& children() const
    {
        return m_children;
    }
    bool is_separator() const {
        return m_name == MenuItemName::Separator;
    }

private:
    MenuItemName m_name;
    const UIItemCommand* m_command{nullptr};
    std::vector<MenuItem*> m_children;
};

} // namespace Slic3r::App
