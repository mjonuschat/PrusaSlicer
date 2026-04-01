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
    set_flags(
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
    );
}

MenuItem* Menu::append_item(
    const std::string& label,
    Render::Icon icon,
    const std::string& shortcut,
    bool action_closes_parent
)
{
    MenuItem* item = emplace_back<MenuItem>(this, label, icon, shortcut, action_closes_parent);

    m_items.push_back(item);
    return item;
}

void Menu::remove_item(size_t index)
{
    remove(m_items.at(index));
    m_items.erase(m_items.cbegin() + index);
}

void Menu::clear()
{
    for (MenuItem* item : m_items) {
        remove_later(item);
    }
    m_items.clear();
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
