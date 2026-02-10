#pragma once

#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class Icon;
class Text;
class Menu;

class MenuItem : public RectangleButton
{
public:
    MenuItem(
        const std::string& label,
        Render::Icon icon           = Render::Icon::None,
        const std::string& shortcut = {},
        bool has_sub_menu           = false
    );
    MenuItem(
        const std::string& label,
        ImColor color_icon_rect,
        const std::string& shortcut = {},
        bool has_sub_menu           = false
    );

    MenuItem* append_sub_menu_item(
        const std::string& label,
        bool* init_checkable_value  = nullptr,
        Render::Icon icon           = Render::Icon::None,
        const std::string& shortcut = {}
    );
    void append_sub_menu_separator();
    void clear_submenu();

    void set_shortcut_internal(const std::string& shortcut) override;

private:
    void create(
        const std::string& label,
        Render::Icon icon           = Render::Icon::None,
        ImColor color_icon_rect     = IM_COL32_BLACK_TRANS,
        const std::string& shortcut = {},
        bool has_sub_menu           = false
    );
    void action_internal() override;

    Icon* m_icon{nullptr};
    Text* m_label{nullptr};
    Menu* m_sub_menu{nullptr};
    Text* m_shortcut_text{nullptr};
};

} // namespace Slic3r::App::Yoga
