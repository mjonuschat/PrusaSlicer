///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/GizmoDialogHelp.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

namespace Slic3r::App::Yoga {

void GizmoDialogHelp::init(Item* container)
{
    m_container = container;
}

void GizmoDialogHelp::add_item(const std::vector<HelpIcon>& icons, const std::string title, bool is_grayed)
{
    ASSERT(m_container);
    ImColor color = ImGui::GetColorU32(is_grayed ? ImGuiCol_TextDisabled : ImGuiCol_Text);
    Item* help_group = m_container->emplace_back<Item>();
    help_group->set_justify_content(YGJustifyCenter);
    help_group->set_align_items(YGAlignCenter);
    help_group->set_gap(5);

    HelpItem help_item;

    int index = 0;
    for (const HelpIcon& help_icon : std::as_const(icons)) {
        ASSERT(help_icon.icon != Render::Icon::None);

        Icon* icon = help_group->emplace_back<Icon>(help_icon.icon);
        icon->set_min_size(help_icon.min_size);
        icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
        if (is_grayed)
            icon->set_tint(color);
        help_item.icons.emplace_back(icon);

        if (++index < icons.size()) {
            Text* text = help_group->emplace_back<Text>("+");
            text->set_text_color(color);
        }
    }
    Text* text = help_group->emplace_back<Text>(title);
    text->set_text_color(color);
    help_item.title = text;

    m_items.emplace_back(help_item);
}

Text* GizmoDialogHelp::title(int item_index)
{
    ASSERT(item_index < m_items.size());
    return m_items[item_index].title;
}

Icon* GizmoDialogHelp::icon(int item_index, int icon_index)
{
    ASSERT(item_index < m_items.size() && icon_index < m_items[item_index].icons.size());
    return m_items[item_index].icons[icon_index];
}

} // namespace Slic3r::App::Yoga
