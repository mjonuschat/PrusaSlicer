#include "Slic3r/App/Yoga/MenuItem.hpp"

#include "Slic3r/App/Yoga/Menu.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"

#include "imgui/imgui_internal.h"

namespace Slic3r::App::Yoga {

MenuItem::MenuItem(
    const std::string& label,
    Render::Icon icon,
    const std::string& shortcut,
    bool has_sub_menu
) :
    RectangleButton()
{
    create(label, icon, IM_COL32_BLACK_TRANS, shortcut, has_sub_menu);
}

MenuItem::MenuItem(
    const std::string& label,
    ImColor color_icon_rect,
    const std::string& shortcut,
    bool has_sub_menu
) :
    RectangleButton()
{
    create(label, Render::Icon::None, color_icon_rect, shortcut, has_sub_menu);
}

void MenuItem::create(
    const std::string& label,
    Render::Icon icon,
    ImColor color_icon_rect,
    const std::string& shortcut,
    bool has_sub_menu
)
{
    if (has_sub_menu) {
        m_sub_menu = emplace_back<Menu>(label, Position::Right);
        m_sub_menu->set_offset(3.f);
    }
    set_background_color(IM_COL32_BLACK_TRANS);

    m_icon = emplace_back<Icon>(icon);
    m_icon->set_aspect_ratio(1);
    // if (icon == Render::Icon::None)
    // render_filled_icon(color_icon_rect);

    m_label = emplace_back<Text>(label);
    m_label->set_flex_grow(1.f);

    float icon_size = 16; // ! for check

    Text* shortcut_text = emplace_back<Text>(shortcut);
    shortcut_text->set_text_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);

    // for expanded menu item use expander icon
    Icon* expander_icon = emplace_back<Icon>(Render::Icon::CloseArrow);
    expander_icon->set_visible(has_sub_menu);
    expander_icon->set_aspect_ratio(1.f);
    expander_icon->set_width(icon_size);
    expander_icon->set_self_align(YGAlignCenter);
}

MenuItem* MenuItem::append_sub_menu_item(
    const std::string& label,
    bool* init_checkable_value,
    Render::Icon icon,
    const std::string& shortcut
)
{
    return m_sub_menu->append_item(label, init_checkable_value, icon, shortcut);
}

void MenuItem::clear_submenu()
{
    // !!? add this as a clear() method fo rthe Item !!?
    // No :))
    for (Item* child : items()) {
        m_sub_menu->remove(child);
    }
}

void MenuItem::action_internal()
{
    if (m_sub_menu) {
        m_sub_menu->open();
    } else {
        Item* parent = parent_item();
        while (parent) {
            Menu* parent_menu = dynamic_cast<Menu*>(parent);
            if (parent_menu) {
                parent_menu->close();
            }
            parent = parent->parent_item();
        }
    }
}

} // namespace Slic3r::App::Yoga
