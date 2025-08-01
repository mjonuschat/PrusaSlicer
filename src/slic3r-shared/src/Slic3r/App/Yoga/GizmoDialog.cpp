///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/GizmoDialog.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {

GizmoDialog::GizmoDialog(const std::string& title)
    : Dialog(title)
{
}

void GizmoDialog::add_separator(Item* item)
{
    Separator* separator = item->emplace_back<Separator>(Orientation::Horizontal);
    separator->set_margin(Margins(-content()->padding().left, 0.f));
}

float GizmoDialog::gap_size() const
{
    return 5.0f;
}

Item* GizmoDialog::add_new_row(const std::string& title, Yoga::ItemPtr controls, YGAlign label_align)
{
    Item* row = content()->emplace_back<Item>();
    row->set_gap(gap_size());
    row->set_padding({10, 0});
    Text* text = row->emplace_back<Text>(title);
    text->set_self_align(label_align);
    text->set_width_percent(35);

    controls->set_width_percent(65);
    row->append(std::move(controls));
    return row;
}

} // namespace Slic3r::App::Yoga
