///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Dialog.hpp"

#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {
constexpr float dialog_padding = 10;

Dialog::Dialog()
{
    WindowPtr window = std::make_unique<Window>("Dialog");

    window->set_orientation(Orientation::Vertical);
    window->set_gap(0);
    window->set_padding(0);
    window->set_flags(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    m_top_row = window->emplace_back<Item>();
    m_top_row->set_max_size({YGUndefined, 40});
    m_top_row->set_flex_shrink(0);

    m_tab_container = m_top_row->emplace_back<Item>();

    m_tab_button_group.callbacks().checked_changed =
        [this](AbstractButton* current_checked, AbstractButton* last_checked) {
        const size_t current_index = std::distance(
            m_tab_buttons.cbegin(),
            std::find(m_tab_buttons.cbegin(), m_tab_buttons.cend(), current_checked)
        );
        on_tab_selected(current_index);
        if (m_callbacks.tab_selected) {
            m_callbacks.tab_selected(current_index);
        }
    };

    Rectangle* buttons_rect = m_top_row->emplace_back<Rectangle>();
    buttons_rect->set_justify_content(YGJustifyFlexEnd);
    buttons_rect->set_align_items(YGAlignCenter);
    buttons_rect->set_padding(dialog_padding);
    buttons_rect->set_fill(m_color_bg_alternate);
    buttons_rect->set_flex_grow(1);
    buttons_rect->set_rounding(0);

    m_close_button = buttons_rect->emplace_back<LayoutButton>("", Render::Icon::PrintIdle);
    m_close_button->set_min_size({20, 20});
    m_close_button->callbacks().action = [this] {
        close();
    };

    m_content = window->emplace_back<Item>();
    m_content->set_padding(dialog_padding);

    set_content_item(std::move(window));
}

Dialog::Dialog(const std::string& tab) : Dialog(std::initializer_list<std::string>{tab}) {}

Dialog::Dialog(std::initializer_list<std::string> tabs) : Dialog()
{
    for (const std::string& tab : tabs) {
        append_tab(tab);
    }
}

Dialog::DialogCallbacks& Dialog::dialog_callbacks()
{
    return m_callbacks;
}

bool Dialog::closable() const
{
    return m_closable;
}

void Dialog::set_closable(bool closable)
{
    if (m_closable != closable) {
        m_closable = closable;
        m_close_button->set_visible(m_closable);
    }
}

Item* Dialog::content() const
{
    return m_content;
}

LayoutButton* Dialog::close_button() const
{
    return m_close_button;
}

Separator* Dialog::add_separator()
{
    Separator* separator = m_content->emplace_back<Separator>(Orientation::Horizontal);
    separator->set_margin(Margins(-dialog_padding, 0.f));
    return separator;
}

LayoutButton* Dialog::append_tab(const std::string& tab)
{
    LayoutButton* tab_button = m_tab_container->emplace_back<LayoutButton>(tab);
    tab_button->set_checkable(true);
    tab_button->set_content_padding({10, 3});
    tab_button->set_background_color(ImColor(41, 41, 41));
    tab_button->set_background_color_checked(ImColor(27, 27, 27));

    if (m_tab_buttons.empty()) {
        tab_button->set_checked(true);
        tab_button->set_draw_flags(ImDrawFlags_RoundCornersTopLeft);
    } else {
        tab_button->set_rounding(0);
    }

    m_tab_buttons.push_back(tab_button);
    m_tab_button_group.insert_button(tab_button);

    return tab_button;
}

void Dialog::set_current_tab(size_t current_index)
{
    m_tab_buttons.at(current_index)->set_checked(true);
}

void Dialog::on_tab_selected(int current_index) {}

} // namespace Slic3r::App::Yoga
