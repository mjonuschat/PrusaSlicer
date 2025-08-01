///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/GizmoDialog.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {

GizmoDialog::GizmoDialog(const std::string& title) : Dialog({title}, "GizmoDialog") {}

void GizmoDialog::add_separator(Item* item)
{
    Separator* separator = item->emplace_back<Separator>(Orientation::Horizontal);
    separator->set_margin(Margins(-content()->padding().left, 0.f));
}

Item* GizmoDialog::add_help(const std::vector<HelpIcon> symbols, const std::string title, Item* help_container, bool is_grayed)
{
    ImColor color = ImGui::GetColorU32(is_grayed ? ImGuiCol_TextDisabled : ImGuiCol_Text);
    Item* help_group = help_container->emplace_back<Item>();
    help_group->set_justify_content(YGJustifyCenter);
    help_group->set_align_items(YGAlignCenter);
    help_group->set_gap(5);

    HelpItem help_item;

    int index = 0;
    for (const HelpIcon& symbol : std::as_const(symbols)) {
        ASSERT(symbol.icon != Render::Icon::None);

        Icon* icon = help_group->emplace_back<Icon>(symbol.icon);
        icon->set_min_size(symbol.min_size);
        icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
        if (is_grayed)
            icon->set_tint(color);
        help_item.icons.emplace_back(icon);

        if (++index < symbols.size()) {
            Text* text = help_group->emplace_back<Text>("+");
            text->set_text_color(color);
        }
    }
    Text* text = help_group->emplace_back<Text>(title);
    text->set_text_color(color);
    help_item.title = text;

    m_help_items.emplace_back(help_item);

    return help_group;
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

Text* GizmoDialog::help_title(int help_item_idx)
{
    ASSERT(help_item_idx < m_help_items.size());
    return m_help_items[help_item_idx].title;
}

Icon* GizmoDialog::help_icon(int help_item_idx, int icon_idx)
{
    ASSERT(help_item_idx < m_help_items.size() && icon_idx < m_help_items[help_item_idx].icons.size());
    return m_help_items[help_item_idx].icons[icon_idx];
}

} // namespace Slic3r::App::Yoga
