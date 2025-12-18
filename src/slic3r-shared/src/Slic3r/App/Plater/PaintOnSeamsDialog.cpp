#include "Slic3r/App/Plater/PaintOnSeamsDialog.hpp"

#include "Slic3r/App/Plater/PaintOnSeamsGizmo.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace Slic3r::App::Plater {

PaintOnSeamsDialog::Callbacks& PaintOnSeamsDialog::callbacks()
{
    return m_callbacks;
}

PaintOnSeamsDialog::PaintOnSeamsDialog() :
    GizmoWindow(_u8L("Paint-on seams"), Render::Icon::PaintSeams)
{
    this->content()->set_orientation(Orientation::Vertical);
    this->content()->set_gap(this->gap_size());

    const Vec2f tool_type_button_size{50.f, 50.f};
    std::unique_ptr<Item> brush_shape_buttons = std::make_unique<Item>();
    brush_shape_buttons->set_gap(this->gap_size());

    m_sphere_brush_button =
        brush_shape_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::Sphere);
    m_sphere_brush_button->set_checkable(true);
    m_sphere_brush_button->set_min_size(tool_type_button_size);
    m_sphere_brush_button->set_content_padding(15);

    m_circle_brush_button =
        brush_shape_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::Circle);
    m_circle_brush_button->set_checkable(true);
    m_circle_brush_button->set_min_size(tool_type_button_size);
    m_circle_brush_button->set_content_padding(15);

    this->add_new_row(_u8L("Brush shape"), std::move(brush_shape_buttons));
    m_brush_shape_group.set_buttons({m_sphere_brush_button, m_circle_brush_button});
    m_brush_shape_group.callbacks().checked_changed =
        [this](AbstractButton* current_checked, AbstractButton* last_checked)
    {
        if (current_checked == m_sphere_brush_button) {
            m_callbacks.brush_shape_changed(Biz::Algorithms::TriangleSelector::CursorType::SPHERE);
        } else if (current_checked == m_circle_brush_button) {
            m_callbacks.brush_shape_changed(Biz::Algorithms::TriangleSelector::CursorType::CIRCLE);
        } else {
            ASSERT(false);
        }
    };

    constexpr float slider_text_size = 50;

    m_brush_radius_slider = Passthrough(std::make_unique<SliderWithInput>());
    m_brush_radius_slider->set_begin_value(PaintOnSeamsGizmo::CursorRadiusMin);
    m_brush_radius_slider->set_end_value(PaintOnSeamsGizmo::CursorRadiusMax);
    m_brush_radius_slider->set_step(0.01);
    m_brush_radius_slider->set_input_width(slider_text_size);
    m_brush_radius_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.brush_radius_changed(value); };
    this->add_new_row(_u8L("Brush size"), m_brush_radius_slider.release());

    this->add_separator(this->content());

    m_clipping_of_view_slider = Passthrough(std::make_unique<SliderWithInput>());
    m_clipping_of_view_slider->set_begin_value(0.);
    m_clipping_of_view_slider->set_end_value(1.);
    m_clipping_of_view_slider->set_step(0.01);
    m_clipping_of_view_slider->set_input_width(slider_text_size);
    m_clipping_of_view_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.clipping_of_view_value_changed(value); };
    this->add_new_row(_u8L("Clipping of view"), m_clipping_of_view_slider.release());

    Item* clipping_of_view_reset_direction_row = this->content()->emplace_back<Item>();
    m_clipping_of_view_reset_direction_button =
        clipping_of_view_reset_direction_row->emplace_back<LayoutButton>(_u8L("Reset direction"));
    m_clipping_of_view_reset_direction_button->callbacks().action = [this]()
    { m_callbacks.clipping_of_view_reset_direction(); };

    this->add_separator(this->content());

    Item* painting_reset_row = this->content()->emplace_back<Item>();
    m_painting_reset_button =
        painting_reset_row->emplace_back<LayoutButton>(_u8L("Remove all selection"));
    m_painting_reset_button->callbacks().action = [this]() { m_callbacks.painting_reset(); };

    this->add_separator(this->content());

    Item* help_row = this->content()->emplace_back<Item>();
    help_row->set_min_size({0, 50});
    help_row->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row->set_align_content(YGAlign::YGAlignCenter);
    help_row->set_padding(5);
    help_row->set_gap(15);
    help_row->set_flex_wrap(YGWrapWrap);

    m_help.init(help_row);
    m_help.add_item({{Render::Icon::MouseLeft}}, _u8L("Paint"));
    m_help.add_item({{Render::Icon::MouseRight}}, _u8L("Block"));
    m_help.add_item(
        {{Render::Icon::KeyShift, {35.f, 35.f}}, {Render::Icon::MouseLeft}},
        _u8L("Remove")
    );
}

void PaintOnSeamsDialog::set_brush_radius(const double brush_radius)
{
    m_brush_radius_slider->set_value(brush_radius);
}

void PaintOnSeamsDialog::set_clipping_of_view_value(const double clipping_of_view_value)
{
    m_clipping_of_view_slider->set_value(clipping_of_view_value);
}

void PaintOnSeamsDialog::set_brush_type(
    const Biz::Algorithms::TriangleSelector::CursorType& brush_type
)
{
    using namespace Slic3r::Biz::Algorithms;

    switch (brush_type) {
    case TriangleSelector::CursorType::SPHERE:
        m_sphere_brush_button->set_checked(true);
        break;
    case TriangleSelector::CursorType::CIRCLE:
        m_circle_brush_button->set_checked(true);
        break;
    default:
        ASSERT(false);
        break;
    }
}

} // namespace Slic3r::App::Plater
