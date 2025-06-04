///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/PaintOnSupportsDialog.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

constexpr float gap_size = 10;

PaintOnSupportsDialog::PaintOnSupportsDialog() : Dialog("Paint-on supports")
{
    const Vec2f button_size{50.f, 50.f};

    set_min_size({325.f, 0});
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(gap_size);

    std::unique_ptr<Item> tool_buttons = std::make_unique<Item>();
    tool_buttons->set_gap(gap_size);
    LayoutButton* brush_button =
        tool_buttons->emplace_back<LayoutButton>("", Render::Icon::PaintBrush);
    brush_button->set_checkable(true);
    brush_button->set_checked(true);
    brush_button->set_min_size(button_size);
    brush_button->set_content_padding(15);
    LayoutButton* magic_wand_button =
        tool_buttons->emplace_back<LayoutButton>("", Render::Icon::WandMagicSparkles);
    magic_wand_button->set_checkable(true);
    magic_wand_button->set_min_size(button_size);
    magic_wand_button->set_content_padding(15);
    add_new_row("Tool", std::move(tool_buttons));
    m_group_tool.set_buttons({brush_button, magic_wand_button});

    std::unique_ptr<Item> brush_shape_buttons = std::make_unique<Item>();
    brush_shape_buttons->set_gap(gap_size);
    LayoutButton* sphere_button =
        brush_shape_buttons->emplace_back<LayoutButton>("", Render::Icon::Sphere);
    sphere_button->set_checkable(true);
    sphere_button->set_checked(true);
    sphere_button->set_min_size(button_size);
    sphere_button->set_content_padding(15);
    LayoutButton* circle_button =
        brush_shape_buttons->emplace_back<LayoutButton>("", Render::Icon::Circle);
    circle_button->set_checkable(true);
    circle_button->set_min_size(button_size);
    circle_button->set_content_padding(15);
    LayoutButton* triangle_button =
        brush_shape_buttons->emplace_back<LayoutButton>("", Render::Icon::Triangle);
    triangle_button->set_checkable(true);
    triangle_button->set_min_size(button_size);
    triangle_button->set_content_padding(15);
    add_new_row("Brush shape", std::move(brush_shape_buttons));
    m_group_shape.set_buttons({sphere_button, circle_button, triangle_button});

    constexpr float slider_text_size = 20;

    std::unique_ptr<Item> brush_size_controls = std::make_unique<Item>();
    brush_size_controls->set_gap(gap_size);
    brush_size_controls->emplace_back<Text>("2.00")->set_width_percent(slider_text_size);
    Rectangle* brush_size_slider = brush_size_controls->emplace_back<Rectangle>();
    brush_size_slider->set_flex_grow(1);
    add_new_row("Brush size", std::move(brush_size_controls));

    add_separator();

    std::unique_ptr<Item> clipping_of_view = std::make_unique<Item>();
    clipping_of_view->set_gap(gap_size);
    clipping_of_view->emplace_back<Text>("0.00")->set_width_percent(slider_text_size);
    Rectangle* clipping_of_view_slider = clipping_of_view->emplace_back<Rectangle>();
    clipping_of_view_slider->set_flex_grow(1);
    add_new_row("Clipping of view", std::move(clipping_of_view));

    std::unique_ptr<Item> show_overhangs = std::make_unique<Item>();
    show_overhangs->set_gap(gap_size);
    show_overhangs->emplace_back<Text>("0")->set_width_percent(slider_text_size);
    Rectangle* show_overhangs_slider = show_overhangs->emplace_back<Rectangle>();
    show_overhangs_slider->set_flex_grow(1);
    add_new_row("Show overhangs", std::move(show_overhangs));

    add_separator();

    content()->emplace_back<ToggleButton>("Paint on overhangs only");
    content()->emplace_back<ToggleButton>("Split triangles");

    add_separator();

    Item* help_row = content()->emplace_back<Item>();
    help_row->set_min_size({0, 50});
    help_row->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row->set_align_content(YGAlign::YGAlignCenter);
    help_row->set_padding(5);

    add_helper({{Render::Icon::MouseLeft, false}}, "Paint", help_row);
    add_helper({{Render::Icon::MouseRight, false}}, "Block", help_row);
    add_helper(
        {{Render::Icon::KeyShift, true}, {Render::Icon::MouseLeft, false}}, "Remove", help_row
    );
}

void PaintOnSupportsDialog::add_new_row(const std::string& title, std::unique_ptr<Item> controls)
{
    Item* row = content()->emplace_back<Item>();
    row->set_gap(gap_size);
    row->set_padding({10, 0});
    Text* text = row->emplace_back<Text>(title);
    text->set_self_align(YGAlignCenter);
    text->set_width_percent(35);

    controls->set_width_percent(65);
    row->append(std::move(controls));
}

Item* PaintOnSupportsDialog::add_helper(
    const std::vector<std::pair<Render::Icon, bool>> symbols, const std::string title, Item* help_row
)
{
    ImColor color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    Item* help_group = help_row->emplace_back<Item>();
    help_group->set_justify_content(YGJustifyCenter);
    help_group->set_align_items(YGAlignCenter);
    help_group->set_gap(5);

    int index = 0;
    for (const std::pair<Render::Icon, bool> symbol : std::as_const(symbols)) {
        Icon* icon = help_group->emplace_back<Icon>(symbol.first);
        icon->set_min_size(symbol.second ? Vec2f{35, 35} : Vec2f{25, 25});
        icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
        if (++index < symbols.size()) {
            Text* text = help_group->emplace_back<Text>("+");
            text->set_text_color(color);
        }
    }
    Text* text = help_group->emplace_back<Text>(title);
    text->set_text_color(color);

    return help_group;
}

} // namespace Slic3r::App::Plater
