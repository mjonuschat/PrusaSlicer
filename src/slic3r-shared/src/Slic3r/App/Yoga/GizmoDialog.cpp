///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/GizmoDialog.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

namespace Slic3r::App::Yoga {
constexpr float dialog_padding = 10;

GizmoDialog::GizmoDialog(const std::string title)
    : Dialog(title)
{
}

void GizmoDialog::add_separator(Item* item)
{
    Separator* separator = item->emplace_back<Separator>(Orientation::Horizontal);
    separator->set_margin(Margins(-dialog_padding, 0.f));
}

Item* GizmoDialog::add_help(const std::vector<HelpIcon> symbols, const std::string title, Item* help_container, bool is_grayed)
{
    ImColor color = ImGui::GetColorU32(is_grayed ? ImGuiCol_TextDisabled : ImGuiCol_Text);
    Item* help_group = help_container->emplace_back<Item>();
    help_group->set_justify_content(YGJustifyCenter);
    help_group->set_align_items(YGAlignCenter);
    help_group->set_gap(5);

    int index = 0;
    for (const HelpIcon& symbol : std::as_const(symbols)) {
        ASSERT(symbol.icon != Render::Icon::None);

        Icon* icon = help_group->emplace_back<Icon>(symbol.icon);
        icon->set_min_size(symbol.min_size);
        icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
        if (is_grayed)
            icon->set_tint(color);

        if (++index < symbols.size()) {
            Text* text = help_group->emplace_back<Text>("+");
            text->set_text_color(color);
        }
    }
    Text* text = help_group->emplace_back<Text>(title);
    text->set_text_color(color);

    return help_group;
}

} // namespace Slic3r::App::Yoga
