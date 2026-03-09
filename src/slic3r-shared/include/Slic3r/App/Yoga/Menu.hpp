#pragma once

#include "Slic3r/App/Yoga/ContextPopup.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class MenuItem;

class Menu : public ContextPopup
{
public:
    Menu(const std::string& name, Position position);

    MenuItem* append_item(
        const std::string& label,
        bool* init_checkable_value  = nullptr,
        Render::Icon icon           = Render::Icon::None,
        const std::string& shortcut = {}
    );
    MenuItem* append_item_as_menu(
        const std::string& label,
        Render::Icon icon           = Render::Icon::None,
        const std::string& shortcut = {}
    );
    void remove_item(size_t index);
    void append_separator();
    void close_all_submenus() const;

private:
    std::vector<MenuItem*> m_items;
};

} // namespace Slic3r::App::Yoga
