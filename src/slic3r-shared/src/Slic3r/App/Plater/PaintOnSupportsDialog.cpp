///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/PaintOnSupportsDialog.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

constexpr float gap_size = 10;

PaintOnSupportsDialog::PaintOnSupportsDialog() : GizmoDialog("Paint-on supports")
{
    const Vec2f button_size{50.f, 50.f};

    content_item()->set_width(325.f);
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

    constexpr float slider_text_size = 50;

    std::unique_ptr<SliderWithInput> brush_size = std::make_unique<SliderWithInput>(7.5f, 2.5f, 0.5f);
    brush_size->set_input_width(slider_text_size);
    add_new_row("Brush size", std::move(brush_size));

    Dialog::add_separator();

    std::unique_ptr<SliderWithInput> clipping_of_view = std::make_unique<SliderWithInput>(2.5f, 7.5f, 0.25f);
    clipping_of_view->set_input_width(slider_text_size);
    add_new_row("Clipping of view", std::move(clipping_of_view));

    std::unique_ptr<SliderWithInput> show_overhangs = std::make_unique<SliderWithInput>(0.f, 3.f);
    show_overhangs->set_input_width(slider_text_size);
    add_new_row("Show overhangs", std::move(show_overhangs));

    Dialog::add_separator();

    content()->emplace_back<ToggleButton>("Paint on overhangs only");
    content()->emplace_back<ToggleButton>("Split triangles");

    Dialog::add_separator();

    Item* help_row = content()->emplace_back<Item>();
    help_row->set_min_size({0, 50});
    help_row->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row->set_align_content(YGAlign::YGAlignCenter);
    help_row->set_padding(5);
    help_row->set_gap(15);

    add_help({{Render::Icon::MouseLeft}}, "Paint", help_row);
    add_help({{Render::Icon::MouseRight}}, "Block", help_row);
    add_help(
        { {Render::Icon::KeyShift, {35.f, 35.f}}, {Render::Icon::MouseLeft} }, "Remove", help_row
    );
}

void PaintOnSupportsDialog::add_new_row(const std::string& title, Yoga::ItemPtr controls)
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

} // namespace Slic3r::App::Plater
