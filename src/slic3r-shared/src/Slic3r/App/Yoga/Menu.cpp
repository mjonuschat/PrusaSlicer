#include "Slic3r/App/Yoga/Menu.hpp"

#include "Slic3r/App/Yoga/MenuItem.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {

Menu::Menu(Item* parent, const std::string& name, Position position)
{    
    WindowPtr window = std::make_unique<Window>(name);

    window->set_orientation(Orientation::Vertical);
    window->set_padding({ 3.f, 10.f });
    window->set_gap(3.f);
    window->set_flags(window->flags() | ImGuiWindowFlags_Tooltip );

    set_content_item(std::move(window));

    attach_to_item(parent, position);
}

MenuItem* Menu::append_item(const std::string& label, bool* init_checkable_value, Render::Icon icon, const std::string& shortcut)
{
    MenuItem* item = content_item()->emplace_back<MenuItem>(label, icon, shortcut);
    item->set_content_justify_content(YGJustifyFlexStart);
    if (init_checkable_value) {
        item->set_checkable(true);
        item->set_checked(*init_checkable_value);
    }

    return item;
}

MenuItem* Menu::append_item_as_menu(const std::string& label, Render::Icon icon, const std::string& shortcut)
{
    MenuItem* item = content_item()->emplace_back<MenuItem>(label, icon, shortcut, true);
    return item;
}

void Menu::append_separator()
{
    Separator* sep = content_item()->emplace_back<Separator>();
    sep->set_fill(ImColor(36, 36, 36));
}

} //namespace Slic3r::App::Yoga 
