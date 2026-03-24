#include "Slic3r/App/Yoga/Menu.hpp"

#include "Slic3r/App/Yoga/MenuItem.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {

Menu::Menu(const std::string& name, Position position)
{
    set_position(position);
    set_orientation(Orientation::Vertical);
    set_padding({3.f, 3.f});
    set_gap(3.f);
}

MenuItem* Menu::append_item(
    const std::string& label,
    bool* init_checkable_value,
    Render::Icon icon,
    const std::string& shortcut
)
{
    MenuItem* item = emplace_back<MenuItem>(this, label, icon, shortcut);
    item->set_content_justify_content(YGJustifyFlexStart);
    if (init_checkable_value) {
        item->set_checkable(true);
        item->set_checked(*init_checkable_value);
    }

    m_items.push_back(item);
    return item;
}

MenuItem*
Menu::append_item_as_menu(const std::string& label, Render::Icon icon, const std::string& shortcut)
{
    MenuItem* item = emplace_back<MenuItem>(this, label, icon, shortcut, true);
    m_items.push_back(item);
    return item;
}

void Menu::remove_item(size_t index)
{
    remove(m_items.at(index));
    m_items.erase(m_items.cbegin() + index);
}

void Menu::append_separator()
{
    emplace_back<Separator>();
}

void Menu::close_all_submenus() const
{
    for (MenuItem* item : m_items) {
        item->close_submenu();
    }
}

} // namespace Slic3r::App::Yoga
