#include "Slic3r/App/Plater/MultiMaterialPaintingDialog.hpp"

#include "Slic3r/App/Plater/MultiMaterialPaintingGizmo.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/Biz/Algorithms/Color.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;
using namespace Slic3r::Biz::Algorithms;

namespace Slic3r::App::Plater {

MultiMaterialPaintingDialog::Callbacks& MultiMaterialPaintingDialog::callbacks()
{
    return m_callbacks;
}

MultiMaterialPaintingDialog::MultiMaterialPaintingDialog(
    const std::vector<Domain::ColorRGBA>& painting_colors
) :
    GizmoWindow(_u8L("Multimaterial painting"), Render::Icon::PaintMultiMaterial)
{
    ASSERT(painting_colors.size() >= 2 && painting_colors.size() <= 16);

    this->content()->set_orientation(Orientation::Vertical);
    this->content()->set_gap(this->gap_size());

    auto add_brush_color_buttons = [&](const std::string& brush_color_label,
                                       std::vector<LayoutButton*>& brush_color_buttons,
                                       ButtonGroup& brush_color_group,
                                       std::function<void(size_t)>& brush_color_group_callback)
    {
        std::unique_ptr<Item> container = std::make_unique<Item>();
        container->set_gap(4.f);

        const Vec2f brush_color_button_size{16.f, 16.f};
        for (const Domain::ColorRGBA& painting_color : painting_colors) {
            const size_t color_idx                = &painting_color - &painting_colors.front();
            const Domain::ColorRGBA checked_color = Color::saturate(painting_color, 0.75f);

            LayoutButton* brush_color_button =
                container->emplace_back<LayoutButton>(std::to_string(color_idx + 1));
            brush_color_button->set_checkable(true);
            brush_color_button->set_min_size(brush_color_button_size);
            brush_color_button->set_content_padding(2.);
            brush_color_button->set_label_font_type(Render::ImguiFontType::Regular);
            brush_color_button->set_background_color(ImColor(
                painting_color.r_uchar(),
                painting_color.g_uchar(),
                painting_color.b_uchar(),
                painting_color.a_uchar()
            ));
            brush_color_button->set_background_color_checked(ImColor(
                checked_color.r_uchar(),
                checked_color.g_uchar(),
                checked_color.b_uchar(),
                checked_color.a_uchar()
            ));

            brush_color_buttons.push_back(brush_color_button);
            brush_color_group.insert_button(brush_color_button);
        }

        this->add_new_row(brush_color_label, std::move(container));
        brush_color_group.callbacks().checked_changed =
            [&brush_color_buttons, &brush_color_group_callback](
                AbstractButton* current_checked,
                AbstractButton* last_checked
            )
        {
            auto it =
                std::find(brush_color_buttons.begin(), brush_color_buttons.end(), current_checked);
            if (it != brush_color_buttons.end()) {
                for (LayoutButton* color_button : brush_color_buttons) {
                    color_button->set_label_font_type(Render::ImguiFontType::Regular);
                }
                (*it)->set_label_font_type(Render::ImguiFontType::Bold);

                brush_color_group_callback(
                    static_cast<size_t>(std::distance(brush_color_buttons.begin(), it))
                );
            }
        };
    };

    add_brush_color_buttons(
        _u8L("First color"),
        m_first_brush_color_buttons,
        m_first_brush_color_group,
        m_callbacks.first_brush_color_changed
    );
    add_brush_color_buttons(
        _u8L("Second color"),
        m_second_brush_color_buttons,
        m_second_brush_color_group,
        m_callbacks.second_brush_color_changed
    );

    this->add_separator(this->content());

    const Vec2f tool_type_button_size{35.f, 35.f};
    std::unique_ptr<Item> tool_type_buttons = std::make_unique<Item>();
    tool_type_buttons->set_gap(gap_size());

    m_brush_button =
        tool_type_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::PaintBrush);
    m_brush_button->set_checkable(true);
    m_brush_button->set_min_size(tool_type_button_size);
    m_brush_button->set_content_padding(5);

    m_smart_fill_button = tool_type_buttons->emplace_back<LayoutButton>(
        std::string{},
        Render::Icon::WandMagicSparkles
    );
    m_smart_fill_button->set_checkable(true);
    m_smart_fill_button->set_min_size(tool_type_button_size);
    m_smart_fill_button->set_content_padding(5);

    m_bucket_fill_button =
        tool_type_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::FillDrip);
    m_bucket_fill_button->set_checkable(true);
    m_bucket_fill_button->set_min_size(tool_type_button_size);
    m_bucket_fill_button->set_content_padding(5);

    m_height_range_button =
        tool_type_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::LineHeight);
    m_height_range_button->set_checkable(true);
    m_height_range_button->set_min_size(tool_type_button_size);
    m_height_range_button->set_content_padding(5);

    this->add_new_row(_u8L("Tool"), std::move(tool_type_buttons));
    m_tool_type_group.set_buttons(
        {m_brush_button, m_smart_fill_button, m_bucket_fill_button, m_height_range_button}
    );
    m_tool_type_group.callbacks().checked_changed =
        [this](AbstractButton* current_checked, AbstractButton* last_checked)
    {
        if (current_checked == m_brush_button) {
            m_selected_tool_type = PaintOnGizmoBase::ToolType::BRUSH;
        } else if (current_checked == m_smart_fill_button) {
            m_selected_tool_type = PaintOnGizmoBase::ToolType::SMART_FILL;
        } else if (current_checked == m_bucket_fill_button) {
            m_selected_tool_type = PaintOnGizmoBase::ToolType::BUCKET_FILL;
        } else if (current_checked == m_height_range_button) {
            m_selected_tool_type = PaintOnGizmoBase::ToolType::HEIGHT_RANGE;
        } else {
            ASSERT(false);
            return;
        }

        this->update_visibility();
        m_callbacks.tool_type_changed(m_selected_tool_type);
    };

    this->add_separator(this->content());

    const Vec2f brush_shape_button_size{50.f, 50.f};
    std::unique_ptr<Item> brush_shape_buttons = std::make_unique<Item>();
    brush_shape_buttons->set_gap(this->gap_size());

    m_sphere_brush_button =
        brush_shape_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::Sphere);
    m_sphere_brush_button->set_checkable(true);
    m_sphere_brush_button->set_min_size(brush_shape_button_size);
    m_sphere_brush_button->set_content_padding(15);

    m_circle_brush_button =
        brush_shape_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::Circle);
    m_circle_brush_button->set_checkable(true);
    m_circle_brush_button->set_min_size(brush_shape_button_size);
    m_circle_brush_button->set_content_padding(15);

    m_triangle_brush_button =
        brush_shape_buttons->emplace_back<LayoutButton>(std::string{}, Render::Icon::Triangle);
    m_triangle_brush_button->set_checkable(true);
    m_triangle_brush_button->set_min_size(brush_shape_button_size);
    m_triangle_brush_button->set_content_padding(15);

    m_brush_shape_row = this->add_new_row(_u8L("Brush shape"), std::move(brush_shape_buttons));
    m_brush_shape_group.set_buttons(
        {m_sphere_brush_button, m_circle_brush_button, m_triangle_brush_button}
    );
    m_brush_shape_group.callbacks().checked_changed =
        [this](AbstractButton* current_checked, AbstractButton* last_checked)
    {
        if (current_checked == m_sphere_brush_button) {
            m_selected_brush_type = TriangleSelector::CursorType::SPHERE;
        } else if (current_checked == m_circle_brush_button) {
            m_selected_brush_type = TriangleSelector::CursorType::CIRCLE;
        } else if (current_checked == m_triangle_brush_button) {
            m_selected_brush_type = TriangleSelector::CursorType::POINTER;
        } else {
            ASSERT(false);
        }

        this->update_visibility();
        m_callbacks.brush_shape_changed(m_selected_brush_type);
    };

    constexpr float slider_text_size = 50;

    m_brush_radius_slider = Passthrough(std::make_unique<SliderWithInput>());
    m_brush_radius_slider->set_begin_value(MultiMaterialPaintingGizmo::CursorRadiusMin);
    m_brush_radius_slider->set_end_value(MultiMaterialPaintingGizmo::CursorRadiusMax);
    m_brush_radius_slider->set_step(0.01);
    m_brush_radius_slider->set_input_width(slider_text_size);
    m_brush_radius_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.brush_radius_changed(value); };
    m_brush_radius_row = this->add_new_row(_u8L("Brush size"), m_brush_radius_slider.release());

    m_split_triangles_toggle = this->content()->emplace_back<ToggleButton>(_u8L("Split triangles"));
    m_split_triangles_toggle->callbacks().checked_changed = [this](bool checked)
    { m_callbacks.split_triangles_value_changed(checked); };

    m_smart_fill_angle_slider = Passthrough(std::make_unique<SliderWithInput>());
    m_smart_fill_angle_slider->set_begin_value(MultiMaterialPaintingGizmo::SmartFillAngleMin);
    m_smart_fill_angle_slider->set_end_value(MultiMaterialPaintingGizmo::SmartFillAngleMax);
    m_smart_fill_angle_slider->set_step(MultiMaterialPaintingGizmo::SmartFillAngleStep);
    m_smart_fill_angle_slider->set_input_width(slider_text_size);
    m_smart_fill_angle_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.smart_fill_angle_changed(value); };
    m_smart_fill_angle_row =
        this->add_new_row(_u8L("Smart fill angle"), m_smart_fill_angle_slider.release());

    m_bucket_fill_angle_slider = Passthrough(std::make_unique<SliderWithInput>());
    m_bucket_fill_angle_slider->set_begin_value(MultiMaterialPaintingGizmo::SmartFillAngleMin);
    m_bucket_fill_angle_slider->set_end_value(MultiMaterialPaintingGizmo::SmartFillAngleMax);
    m_bucket_fill_angle_slider->set_step(MultiMaterialPaintingGizmo::SmartFillAngleStep);
    m_bucket_fill_angle_slider->set_input_width(slider_text_size);
    m_bucket_fill_angle_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.bucket_fill_angle_changed(value); };
    m_bucket_fill_angle_row =
        this->add_new_row(_u8L("Bucket fill angle"), m_bucket_fill_angle_slider.release());

    // TODO: Add mm suffix to the slider label.
    m_height_range_slider = Passthrough(std::make_unique<SliderWithInput>());
    m_height_range_slider->set_begin_value(MultiMaterialPaintingGizmo::HeightRangeZRangeMin);
    m_height_range_slider->set_end_value(MultiMaterialPaintingGizmo::HeightRangeZRangeMax);
    m_height_range_slider->set_step(MultiMaterialPaintingGizmo::HeightRangeZRangeStep);
    m_height_range_slider->set_input_width(slider_text_size);
    m_height_range_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.height_range_changed(value); };
    m_height_range_row = this->add_new_row(_u8L("Height range"), m_height_range_slider.release());

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
    m_help.add_item({{Render::Icon::MouseLeft}}, _u8L("First color"));
    m_help.add_item({{Render::Icon::MouseRight}}, _u8L("Second color"));
    m_help.add_item(
        {{Render::Icon::KeyShift, {35.f, 35.f}}, {Render::Icon::MouseLeft}},
        _u8L("Remove")
    );
}

void MultiMaterialPaintingDialog::set_brush_radius(const double brush_radius)
{
    m_brush_radius_slider->set_value(brush_radius);
}

void MultiMaterialPaintingDialog::set_clipping_of_view_value(const double clipping_of_view_value)
{
    m_clipping_of_view_slider->set_value(clipping_of_view_value);
}

void MultiMaterialPaintingDialog::set_smart_fill_angle(const double smart_fill_angle)
{
    m_smart_fill_angle_slider->set_value(smart_fill_angle);
}

void MultiMaterialPaintingDialog::set_bucket_fill_angle(double bucket_fill_angle)
{
    m_bucket_fill_angle_slider->set_value(bucket_fill_angle);
}

void MultiMaterialPaintingDialog::set_height_range(double height_range)
{
    m_height_range_slider->set_value(height_range);
}

void MultiMaterialPaintingDialog::set_split_triangles_value(const bool split_triangles)
{
    m_split_triangles_toggle->set_checked(split_triangles);
}

void MultiMaterialPaintingDialog::set_first_brush_color_index(size_t color_idx)
{
    ASSERT(color_idx < m_first_brush_color_buttons.size());
    m_first_brush_color_buttons[color_idx]->set_checked(true);
    m_first_brush_color_buttons[color_idx]->set_label_font_type(Render::ImguiFontType::Bold);
}

void MultiMaterialPaintingDialog::set_second_brush_color_index(size_t color_idx)
{
    ASSERT(color_idx < m_second_brush_color_buttons.size());
    m_second_brush_color_buttons[color_idx]->set_checked(true);
    m_second_brush_color_buttons[color_idx]->set_label_font_type(Render::ImguiFontType::Bold);
}

void MultiMaterialPaintingDialog::set_tool_type(const PaintOnGizmoBase::ToolType& tool_type)
{
    switch (tool_type) {
    case PaintOnGizmoBase::ToolType::BRUSH:
        m_brush_button->set_checked(true);
        break;
    case PaintOnGizmoBase::ToolType::SMART_FILL:
        m_smart_fill_button->set_checked(true);
        break;
    default:
        ASSERT(false);
        break;
    }

    this->update_visibility();
}

void MultiMaterialPaintingDialog::set_brush_type(
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
    case TriangleSelector::CursorType::POINTER:
        m_triangle_brush_button->set_checked(true);
        break;
    default:
        ASSERT(false);
        break;
    }

    this->update_visibility();
}

void MultiMaterialPaintingDialog::update_visibility()
{
    if (m_selected_tool_type == PaintOnGizmoBase::ToolType::BRUSH) {
        m_brush_shape_row->set_visible(true);

        if (m_selected_brush_type == TriangleSelector::CursorType::POINTER) {
            m_brush_radius_row->set_visible(false);
            m_split_triangles_toggle->set_visible(false);
        } else {
            m_brush_radius_row->set_visible(true);
            m_split_triangles_toggle->set_visible(true);
        }

        m_smart_fill_angle_row->set_visible(false);
        m_bucket_fill_angle_row->set_visible(false);
        m_height_range_row->set_visible(false);
    } else if (m_selected_tool_type == PaintOnGizmoBase::ToolType::SMART_FILL) {
        m_brush_shape_row->set_visible(false);
        m_brush_radius_row->set_visible(false);
        m_split_triangles_toggle->set_visible(false);
        m_bucket_fill_angle_row->set_visible(false);
        m_height_range_row->set_visible(false);

        m_smart_fill_angle_row->set_visible(true);
    } else if (m_selected_tool_type == PaintOnGizmoBase::ToolType::BUCKET_FILL) {
        m_brush_shape_row->set_visible(false);
        m_brush_radius_row->set_visible(false);
        m_split_triangles_toggle->set_visible(false);
        m_smart_fill_angle_row->set_visible(false);
        m_height_range_row->set_visible(false);

        m_bucket_fill_angle_row->set_visible(true);
    } else if (m_selected_tool_type == PaintOnGizmoBase::ToolType::HEIGHT_RANGE) {
        m_brush_shape_row->set_visible(false);
        m_brush_radius_row->set_visible(false);
        m_split_triangles_toggle->set_visible(false);
        m_smart_fill_angle_row->set_visible(false);
        m_bucket_fill_angle_row->set_visible(false);

        m_height_range_row->set_visible(true);
    }
}

} // namespace Slic3r::App::Plater
