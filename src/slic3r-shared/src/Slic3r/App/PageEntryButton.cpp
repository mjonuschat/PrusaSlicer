///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PageEntryButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PageEntryButton::PageEntryButton(
    size_t index,
    const PageEntry& page_entry,
    FnIndexClicked on_clicked
) :
    LayoutButton(page_entry.name, page_entry.icon),
    Biz::DataObserver<PageEntry>(index, page_entry),
    m_on_clicked(on_clicked)
{
    ASSERT(m_on_clicked);

    callbacks().action = [this]() { m_on_clicked(m_index); };

    set_rounding(0);
    set_content_justify_content(YGJustifyFlexStart);

    set_content_padding({5, 10});

    set_background_color(
        IM_COL32_BLACK_TRANS,
        m_theme->color_imgui(Platform::Color::Button, Platform::ColorGroup::Hovered)
    );
}

void PageEntryButton::on_data_update()
{
    set_label(m_state->name);
    set_icon(m_state->icon);
}

} // namespace Slic3r::App
