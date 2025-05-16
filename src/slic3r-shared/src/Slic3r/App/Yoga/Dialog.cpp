///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/Dialog.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {
constexpr float dialog_padding = 10;

Dialog::Dialog(const std::string title)
    : AttachedWindow(title, Position::Right)
{
    set_orientation(Orientation::Vertical);
    set_gap(0);
    set_padding(0);

    Item* top_row = emplace_back<Item>();
    top_row->set_max_size({YGUndefined, 40});

    Item* title_container = top_row->emplace_back<Item>();
    title_container->set_justify_content(YGJustifyCenter);
    title_container->set_align_items(YGAlignCenter);
    title_container->set_width_percent(50);

    m_title = title_container->emplace_back<Text>(title);

    Rectangle* buttons_rect = top_row->emplace_back<Rectangle>();
    buttons_rect->set_justify_content(YGJustifyFlexEnd);
    buttons_rect->set_align_items(YGAlignCenter);
    buttons_rect->set_padding(dialog_padding);
    buttons_rect->set_fill(m_color_bg_alternate);
    buttons_rect->set_width_percent(50);
    buttons_rect->set_rounding(0);

    m_close_button = buttons_rect->emplace_back<LayoutButton>("", ImGui::PrintIdle);
    m_close_button->set_min_size({20, 20});
    m_close_button->callbacks().action = [this] { set_visible(false); };

    m_content = emplace_back<Item>();
    m_content->set_padding(dialog_padding);
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

} // namespace Slic3r::App::Yoga
