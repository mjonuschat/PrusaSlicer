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

Dialog::Dialog(const std::string& tab) : Dialog(std::initializer_list<std::string>{tab}) {}

Dialog::Dialog(std::initializer_list<std::string> tabs)
{
    ASSERT(tabs.size());

    WindowPtr window = std::make_unique<Window>(*tabs.begin());

    window->set_orientation(Orientation::Vertical);
    window->set_gap(0);
    window->set_padding(0);
    window->set_flags(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    Item* top_row = window->emplace_back<Item>();
    top_row->set_max_size({YGUndefined, 40});
    top_row->set_flex_shrink(0);

    bool first = true;
    for (const std::string& tab : tabs) {
        LayoutButton* tab_button = top_row->emplace_back<LayoutButton>(tab);
        tab_button->set_checkable(true);
        tab_button->set_content_padding({10, 3});
        tab_button->set_background_color(ImColor(41, 41, 41));
        tab_button->set_background_color_checked(ImColor(27, 27, 27));
        m_tab_buttons.push_back(tab_button);
        m_tab_button_group.insert_button(tab_button);

        if (first) {
            first = false;
            tab_button->set_checked(true);
            tab_button->set_draw_flags(ImDrawFlags_RoundCornersTopLeft);
        } else {
            tab_button->set_rounding(0);
        }
    }

    m_tab_button_group.callbacks().checked_changed =
        [this](AbstractButton* current_checked, AbstractButton* last_checked) {
            on_tab_selected(
                std::distance(
                    m_tab_buttons.cbegin(),
                    std::find(m_tab_buttons.cbegin(), m_tab_buttons.cend(), current_checked)
                )
            );
        };

    Rectangle* buttons_rect = top_row->emplace_back<Rectangle>();
    buttons_rect->set_justify_content(YGJustifyFlexEnd);
    buttons_rect->set_align_items(YGAlignCenter);
    buttons_rect->set_padding(dialog_padding);
    buttons_rect->set_fill(m_color_bg_alternate);
    buttons_rect->set_flex_grow(1);
    buttons_rect->set_rounding(0);

    m_close_button = buttons_rect->emplace_back<LayoutButton>("", Render::Icon::PrintIdle);
    m_close_button->set_min_size({20, 20});
    m_close_button->callbacks().action = [this] { close(); };

    m_content = window->emplace_back<Item>();
    m_content->set_padding(dialog_padding);

    set_content_item(std::move(window));
}

bool Dialog::closable() const { return m_closable; }

void Dialog::set_closable(bool closable)
{
    if (m_closable != closable) {
        m_closable = closable;
        m_close_button->set_visible(m_closable);
    }
}

Item* Dialog::content() const { return m_content; }

void Dialog::add_separator()
{
    Separator* separator = m_content->emplace_back<Separator>(Orientation::Horizontal);
    separator->set_margin(Margins(-dialog_padding, 0.f));
}

void Dialog::set_current_tab(size_t current_index)
{
    m_tab_buttons.at(current_index)->set_checked(true);
}

void Dialog::on_tab_selected(int current_index) {}

} // namespace Slic3r::App::Yoga
