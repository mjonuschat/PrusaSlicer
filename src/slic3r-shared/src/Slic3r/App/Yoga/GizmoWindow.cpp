///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/GizmoWindow.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"

namespace Slic3r::App::Yoga {

constexpr float dialog_padding = 10;

GizmoWindow::GizmoWindow(const std::string& title, Render::Icon icon) : Window("GizmoWindow")
{
    set_orientation(Orientation::Vertical);
    set_gap(0);
    set_padding(0);
    set_flex_grow(1);
    set_flags(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    Item* m_top_row = emplace_back<Item>();
    m_top_row->set_max_size({YGUndefined, 40});
    m_top_row->set_flex_shrink(0);

    Rectangle* buttons_rect = m_top_row->emplace_back<Rectangle>();
    buttons_rect->set_align_items(YGAlignCenter);
    buttons_rect->set_padding(dialog_padding);
    buttons_rect->set_fill(m_color_bg_alternate);
    buttons_rect->set_flex_grow(1);
    buttons_rect->set_flags(ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);

    Icon* header_icon = buttons_rect->emplace_back<Icon>(icon);
    header_icon->set_margin(Margins{0, 0, 3, 0});
    header_icon->set_width(20);
    header_icon->set_height(20);

    Text* title_text = buttons_rect->emplace_back<Text>(title);
    title_text->set_font_type(Render::ImguiFontType::Bold);

    Item* spacer = buttons_rect->emplace_back<Item>();
    spacer->set_flex_grow(1);

    m_close_button = buttons_rect->emplace_back<LayoutButton>("", Render::Icon::PrintIdle);
    m_close_button->set_min_size({20, 20});
    m_close_button->callbacks().action = [this]
    {
        if (m_gizmo_callback.close_requested) {
            m_gizmo_callback.close_requested();
        }
    };

    m_content = emplace_back<Item>();
    m_content->set_padding(dialog_padding);
}

GizmoWindow::GizmoCallbacks& GizmoWindow::gizmo_callbacks()
{
    return m_gizmo_callback;
}

Separator* GizmoWindow::add_separator(Item* item)
{
    Separator* separator = item->emplace_back<Separator>(Orientation::Horizontal);

    float margin_begin{0.f};
    float margin_end{0.f};
    Item* parent_item = item;
    while (parent_item != this) {
        margin_begin += item->orientation() == Orientation::Vertical ? parent_item->padding().left :
                                                                       parent_item->padding().top;
        margin_end += item->orientation() == Orientation::Vertical ? parent_item->padding().right :
                                                                     parent_item->padding().bottom;
        parent_item = parent_item->parent_item();
    }

    if (item->orientation() == Orientation::Vertical) {
        separator->set_margin(Margins(-margin_begin, 0.f, -margin_end, 0.f));
    } else {
        separator->set_margin(Margins(0.f, -margin_begin, 0.f, -margin_end));
    }
    return separator;
}

float GizmoWindow::gap_size() const
{
    return 5.0f;
}

Item*
GizmoWindow::add_new_row(const std::string& title, Yoga::ItemPtr controls, YGAlign label_align)
{
    Item* row = content()->emplace_back<Item>();
    row->set_gap(gap_size());
    row->set_padding({10, 0});
    Text* text = row->emplace_back<Text>(title);
    text->set_self_align(label_align);
    text->set_width(100);

    controls->set_flex_grow(1);
    row->append(std::move(controls));
    return row;
}

Item* GizmoWindow::content() const
{
    return m_content;
}

LayoutButton* GizmoWindow::close_button() const
{
    return m_close_button;
}

} // namespace Slic3r::App::Yoga
