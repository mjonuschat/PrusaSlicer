#include "Slic3r/App/Plater/MultiMaterialPaintingDialog.hpp"

#include "Slic3r/App/Plater/MultiMaterialPaintingGizmo.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Plater/MMPaintingScaleHelpers.hpp"
#include "Slic3r/App/Plater/MMPaintingUtils.hpp"
#include "Slic3r/App/Plater/MMPaintingColorDropdowns.hpp"
#include "Slic3r/App/Plater/MMPaintingColorSelector.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;
using namespace Slic3r::Biz::Algorithms;

namespace Slic3r::App::Plater {

MultiMaterialPaintingDialog::Callbacks& MultiMaterialPaintingDialog::callbacks()
{
    return m_callbacks;
}

static const ImColor mouse_left_color{115, 151, 236};
static const ImColor mouse_right_color{175, 119, 255};
const float spacing{5_px};
const float prefered_icon_size{36_px};

Yoga::ItemPtr label(const std::string& text)
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    result->set_flex_grow(1);
    result->set_orientation(Orientation::Horizontal);
    result->set_align_items(YGAlignCenter);
    result->set_justify_content(YGJustifyFlexStart);
    result->emplace_back<Yoga::Text>(text);
    return result;
}

void apply_icon_button_style(Yoga::Item& item)
{
    item.set_align_items(YGAlignStretch);
    item.set_min_size({14_px, 14_px});
    item.set_max_size({prefered_icon_size, prefered_icon_size});
    item.set_flex_grow(1);
    item.set_aspect_ratio(1);
}

class IconButton : public RectangleButton
{
public:
    IconButton(Render::Icon icon, const std::string& tooltip)
    {
        m_icon = emplace_back<Icon>(icon);
        m_icon->set_aspect_ratio(1);
        m_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
        m_icon->set_width(14_px);
        m_icon->set_height(14_px);

        apply_icon_button_style(*this);
        set_content_align_items(YGAlignCenter);
        set_checkable(true);
        set_content_padding(0);
        set_tooltip(tooltip);
    }

    Icon* m_icon = nullptr;
};

Yoga::ItemPtr icons_row()
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    result->set_orientation(Orientation::Horizontal);
    result->set_height(prefered_icon_size);
    result->set_align_items(YGAlignCenter);
    result->set_gap(2 * spacing);
    result->set_justify_content(YGJustifyFlexStart);
    return result;
}

Yoga::ItemPtr MultiMaterialPaintingDialog::brush_properties_picker()
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    result->set_gap(4 * spacing);
    auto labels{result->emplace_back<Item>()};
    labels->set_orientation(Orientation::Vertical);
    labels->set_gap(3 * spacing);

    Item* tools_label{Plater::append(labels, label(_u8("Tool")))};
    tools_label->set_flex_grow(1);

    m_shapes_label = Plater::append(labels, label(_u8("Shape")));
    m_shapes_label->set_flex_grow(1);

    auto icons{result->emplace_back<Item>()};
    icons->set_orientation(Orientation::Vertical);
    icons->set_align_items(YGAlignStretch);
    icons->set_gap(3 * spacing);
    icons->set_flex_grow(1);

    Item* tools_row{Plater::append(icons, icons_row())};

    m_brush_button = tools_row->emplace_back<IconButton>(Render::Icon::PaintBrush, _u8L("Brush"));
    m_smart_fill_button =
        tools_row->emplace_back<IconButton>(Render::Icon::WandMagicSparkles, _u8L("Smart fill"));
    m_bucket_fill_button =
        tools_row->emplace_back<IconButton>(Render::Icon::FillDrip, _u8L("Bucket fill"));
    m_height_range_button =
        tools_row->emplace_back<IconButton>(Render::Icon::LineHeight, _u8L("Height range fill"));

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
        }

        update_visibility();
        m_callbacks.tool_type_changed(m_selected_tool_type);
    };

    m_brush_shape_row = Plater::append(icons, icons_row());

    m_sphere_brush_button =
        m_brush_shape_row->emplace_back<IconButton>(Render::Icon::Sphere, _u8L("Sphere"));
    m_circle_brush_button =
        m_brush_shape_row->emplace_back<IconButton>(Render::Icon::Circle, _u8L("Circle"));
    m_triangle_brush_button =
        m_brush_shape_row->emplace_back<IconButton>(Render::Icon::Triangle, _u8L("Trianle"));
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

    auto spacer{m_brush_shape_row->emplace_back<Item>()};
    apply_icon_button_style(*spacer);

    return result;
}

Yoga::ItemPtr MultiMaterialPaintingDialog::brush_size_picker()
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    m_brush_radius_row = result.get();
    result->set_orientation(Orientation::Vertical);
    result->set_gap(2 * spacing);

    auto label{result->emplace_back<Yoga::Text>(_u8L("Brush size"))};
    label->set_font_type(Render::ImguiFontType::Bold);

    m_brush_radius_slider = result->emplace_back<SliderWithInput>(_u8L("mm"));
    m_brush_radius_slider->set_begin_value(MultiMaterialPaintingGizmo::CursorRadiusMin);
    m_brush_radius_slider->set_end_value(MultiMaterialPaintingGizmo::CursorRadiusMax);
    m_brush_radius_slider->set_step(0.01);

    m_brush_radius_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.brush_radius_changed(value); };
    return result;
}

Yoga::ItemPtr MultiMaterialPaintingDialog::smart_fill_angle_picker()
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    m_smart_fill_angle_row = result.get();
    result->set_orientation(Orientation::Vertical);
    result->set_gap(2 * spacing);

    auto label{result->emplace_back<Yoga::Text>(_u8L("Smart fill angle"))};
    label->set_font_type(Render::ImguiFontType::Bold);

    m_smart_fill_angle_slider = result->emplace_back<SliderWithInput>(_u8L("°"));
    m_smart_fill_angle_slider->set_begin_value(MultiMaterialPaintingGizmo::SmartFillAngleMin);
    m_smart_fill_angle_slider->set_end_value(MultiMaterialPaintingGizmo::SmartFillAngleMax);
    m_smart_fill_angle_slider->set_step(MultiMaterialPaintingGizmo::SmartFillAngleStep);
    m_smart_fill_angle_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.smart_fill_angle_changed(value); };
    return result;
}

Yoga::ItemPtr MultiMaterialPaintingDialog::bucket_fill_angle_picker()
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    m_bucket_fill_angle_row = result.get();
    result->set_orientation(Orientation::Vertical);
    result->set_gap(2 * spacing);

    auto label{result->emplace_back<Yoga::Text>(_u8L("Bucket fill angle"))};
    label->set_font_type(Render::ImguiFontType::Bold);

    m_bucket_fill_angle_slider = result->emplace_back<SliderWithInput>(_u8L("°"));
    m_bucket_fill_angle_slider->set_begin_value(MultiMaterialPaintingGizmo::SmartFillAngleMin);
    m_bucket_fill_angle_slider->set_end_value(MultiMaterialPaintingGizmo::SmartFillAngleMax);
    m_bucket_fill_angle_slider->set_step(MultiMaterialPaintingGizmo::SmartFillAngleStep);
    m_bucket_fill_angle_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.bucket_fill_angle_changed(value); };
    return result;
}

Yoga::ItemPtr MultiMaterialPaintingDialog::height_range_picker()
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    m_height_range_row = result.get();
    result->set_orientation(Orientation::Vertical);
    result->set_gap(2 * spacing);

    auto label{result->emplace_back<Yoga::Text>(_u8L("Heiht range"))};
    label->set_font_type(Render::ImguiFontType::Bold);

    m_height_range_slider = result->emplace_back<SliderWithInput>(_u8L("mm"));
    m_height_range_slider->set_begin_value(MultiMaterialPaintingGizmo::HeightRangeZRangeMin);
    m_height_range_slider->set_end_value(MultiMaterialPaintingGizmo::HeightRangeZRangeMax);
    m_height_range_slider->set_step(MultiMaterialPaintingGizmo::HeightRangeZRangeStep);
    m_height_range_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.height_range_changed(value); };
    return result;
}

static ItemPtr numbers_help()
{
    GizmoDialogHelp numbers_help;
    auto result{std::make_unique<Yoga::Item>()};
    auto numbers{result->emplace_back<Item>()};
    numbers->set_gap(5_px);
    numbers->set_align_items(YGAlignCenter);
    numbers_help.init(numbers);

    const std::vector<Yoga::GizmoDialogHelp::HelpIcon> help_icons{
        {Render::Icon::KeyShift, {37_px, 14_px}},
        {Render::Icon::Key1, {16_px, 14_px}}
    };
    numbers_help.add_item(help_icons, "/");
    numbers_help.add_item({{Render::Icon::KeyDots, {18_px, 14_px}}}, "/");
    numbers_help.add_item({{Render::Icon::Key8, {16_px, 14_px}}}, "Set 1. color");
    return result;
}

static Yoga::ItemPtr help()
{
    Yoga::ItemPtr result{std::make_unique<Yoga::Item>()};
    result->set_orientation(Orientation::Vertical);
    result->set_gap(2 * spacing);
    GizmoDialogHelp help;
    help.init(result.get());
    Domain::Vec2f icon_size{16_px, 16_px};
    help.add_item({{Render::Icon::MouseLeft, icon_size}}, _u8L("Paint 1. color"));
    help.add_item({{Render::Icon::MouseRight, icon_size}}, _u8L("Paint 2. color"));
    help.add_item(
        {{Render::Icon::KeyShift, {37_px, 14_px}}, {Render::Icon::MouseLeft, icon_size}},
        _u8L("Remove painted color")
    );
    help.add_item({{Render::Icon::KeyX, {17_px, 14_px}}}, _u8L("Swap colors"));

    append(result.get(), numbers_help());

    help.add_item(
        {{Render::Icon::KeyAlt, {27_px, 14_px}}, {Render::Icon::MouseWheel, icon_size}},
        _u8L("Brush size")
    );
    return result;
}

MultiMaterialPaintingDialog::MultiMaterialPaintingDialog() :
    GizmoWindow(_u8L("Painting"), Render::Icon::None, _u8L("N"))
{
    const float padding{4 * spacing};

    content()->set_orientation(Orientation::Vertical);
    content()->set_flex_grow(1);

    auto scroll_area{content()->emplace_back<Yoga::ScrollArea>("ScrollPanels")};
    scroll_area->set_gap(0);
    scroll_area->set_orientation(Orientation::Vertical);
    scroll_area->set_flex_grow(1);
    scroll_area->set_padding(0);
    scroll_area->set_max_size({400, YGUndefined});

    revert_button()->set_visible(true);
    revert_button()->callbacks().action = [this]() { m_callbacks.painting_reset(); };
    revert_button()->set_tooltip(_u8L("Reset painting"));

    auto selector_section{scroll_area->emplace_back<Item>()};
    selector_section->set_padding(padding);
    selector_section->set_orientation(Orientation::Vertical);
    selector_section->set_gap(3 * spacing);
    selector_section->set_flex_shrink(0);

    m_color_dropdowns = selector_section->emplace_back<ColorDropdowns>(
        spacing,
        mouse_left_color,
        mouse_right_color
    );
    m_color_selector =
        selector_section->emplace_back<ColorSelector>(mouse_left_color, mouse_right_color);

    auto on_color_selected{[this](SelectedColor color, std::size_t index)
                           {
                               if (color == SelectedColor::Primary) {
                                   callbacks().first_brush_color_changed(index);
                               }
                               if (color == SelectedColor::Secondary) {
                                   callbacks().second_brush_color_changed(index);
                               }
                           }};

    m_color_dropdowns->on_color_selected =
        [this, on_color_selected](SelectedColor color, std::size_t index)
    {
        m_color_selector->select_color_index(color, index);
        on_color_selected(color, index);
    };
    m_color_selector->on_color_selected =
        [this, on_color_selected](SelectedColor color, std::size_t index)
    {
        m_color_dropdowns->select_color_index(color, index);
        on_color_selected(color, index);
    };

    add_separator(scroll_area);

    auto brush_section{scroll_area->emplace_back<Item>()};
    brush_section->set_padding(padding);
    brush_section->set_orientation(Orientation::Vertical);
    brush_section->set_gap(padding);
    brush_section->set_flex_shrink(0);

    Plater::append(brush_section, brush_properties_picker());
    Plater::append(brush_section, brush_size_picker());
    Plater::append(brush_section, smart_fill_angle_picker());
    Plater::append(brush_section, bucket_fill_angle_picker());
    Plater::append(brush_section, height_range_picker());

    add_separator(scroll_area);

    auto view_clipper_section{scroll_area->emplace_back<Item>()};
    view_clipper_section->set_padding(padding);
    view_clipper_section->set_orientation(Orientation::Vertical);
    view_clipper_section->set_gap(2 * spacing);
    view_clipper_section->set_gap(spacing);
    view_clipper_section->set_flex_shrink(0);
    auto view_clipper_label{
        view_clipper_section->emplace_back<Yoga::Text>(_u8L("Clipping of view"))
    };

    auto view_clipper_slider_row{view_clipper_section->emplace_back<Item>()};
    view_clipper_slider_row->set_gap(spacing);
    view_clipper_slider_row->set_align_items(YGAlignCenter);
    view_clipper_slider_row->set_width_percent(100);
    view_clipper_slider_row->set_flex_shrink(0);

    view_clipper_label->set_font_type(Render::ImguiFontType::Bold);
    m_clipping_of_view_reset_direction_button = view_clipper_slider_row->emplace_back<LayoutButton>(
        "",
        Render::Icon::ArrowUpToLine,
        _u8L("Reset clipping direction")
    );
    m_clipping_of_view_reset_direction_button->set_height_percent(80);
    m_clipping_of_view_reset_direction_button->set_content_padding(3_px);
    m_clipping_of_view_reset_direction_button->callbacks().action = [this]()
    { m_callbacks.clipping_of_view_reset_direction(); };

    m_clipping_of_view_slider = view_clipper_slider_row->emplace_back<SliderWithInput>();
    m_clipping_of_view_slider->set_flex_grow(1);
    m_clipping_of_view_slider->set_begin_value(0.);
    m_clipping_of_view_slider->set_end_value(1.);
    m_clipping_of_view_slider->set_step(0.01);
    m_clipping_of_view_slider->callbacks().value_changed = [this](double value)
    { m_callbacks.clipping_of_view_value_changed(value); };

    add_separator(scroll_area);

    m_split_triangles_section = scroll_area->emplace_back<Item>();
    m_split_triangles_section->set_padding(padding);
    m_split_triangles_section->set_orientation(Orientation::Horizontal);
    m_split_triangles_section->set_align_items(YGAlignFlexEnd);
    m_split_triangles_section->set_gap(2 * spacing);
    m_split_triangles_section->set_flex_shrink(0);
    m_split_triangles_toggle = m_split_triangles_section->emplace_back<Yoga::ToggleButton>();
    m_split_triangles_toggle->callbacks().checked_changed = [this](bool checked)
    { m_callbacks.split_triangles_value_changed(checked); };
    m_split_triangles_section->emplace_back<Yoga::Text>(_u8L("Split triangles"));

    m_split_triangles_separator = add_separator(scroll_area);

    auto help_section{scroll_area->emplace_back<Item>()};
    help_section->set_padding(padding);
    help_section->set_flex_shrink(0);
    Plater::append(help_section, help());
}

void MultiMaterialPaintingDialog::set_brush_radius(const double brush_radius)
{
    m_brush_radius_slider->set_value(brush_radius);
}

void MultiMaterialPaintingDialog::set_first_brush_color_index(size_t color_idx)
{
    if (color_idx >= m_color_selector->colors_count()) {
        return;
    }
    m_color_selector->select_color_index(SelectedColor::Primary, color_idx);
    m_color_dropdowns->select_color_index(SelectedColor::Primary, color_idx);
}

void MultiMaterialPaintingDialog::set_second_brush_color_index(size_t color_idx)
{
    if (color_idx >= m_color_selector->colors_count()) {
        return;
    }
    m_color_selector->select_color_index(SelectedColor::Secondary, color_idx);
    m_color_dropdowns->select_color_index(SelectedColor::Secondary, color_idx);
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

void MultiMaterialPaintingDialog::set_painting_colors(
    const std::vector<Domain::ColorRGBA>& colors,
    const std::vector<std::string>& names
)
{
    m_color_selector->set_colors(colors);
    m_color_dropdowns->set_items(names, colors);
}

void MultiMaterialPaintingDialog::switch_colors()
{
    m_color_dropdowns->switch_colors();
    auto [primary_color_index, secondary_color_index]{m_color_dropdowns->current_indicies()};
    m_color_selector->select_color_index(SelectedColor::Primary, primary_color_index);
    m_color_selector->select_color_index(SelectedColor::Secondary, secondary_color_index);
    m_callbacks.first_brush_color_changed(primary_color_index);
    m_callbacks.second_brush_color_changed(secondary_color_index);
}

void MultiMaterialPaintingDialog::set_tool_type(const PaintOnGizmoBase::ToolType& tool_type)
{
    switch (tool_type) {
    case PaintOnGizmoBase::ToolType::BRUSH:
        m_brush_button->set_checked(true);
        break;
    case PaintOnGizmoBase::ToolType::BUCKET_FILL:
        m_bucket_fill_button->set_checked(true);
        break;
    case PaintOnGizmoBase::ToolType::SMART_FILL:
        m_smart_fill_button->set_checked(true);
        break;
    case PaintOnGizmoBase::ToolType::HEIGHT_RANGE:
        m_height_range_button->set_checked(true);
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
    return;
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
    m_brush_shape_row->set_visible(false);
    m_shapes_label->set_visible(false);
    m_brush_radius_row->set_visible(false);
    m_split_triangles_section->set_visible(false);
    m_split_triangles_separator->set_visible(false);
    m_smart_fill_angle_row->set_visible(false);
    m_bucket_fill_angle_row->set_visible(false);
    m_height_range_row->set_visible(false);

    if (m_selected_tool_type == PaintOnGizmoBase::ToolType::BRUSH) {
        m_brush_shape_row->set_visible(true);
        m_shapes_label->set_visible(true);

        if (m_selected_brush_type != TriangleSelector::CursorType::POINTER) {
            m_brush_radius_row->set_visible(true);
            m_split_triangles_section->set_visible(true);
            m_split_triangles_separator->set_visible(true);
        }
    } else if (m_selected_tool_type == PaintOnGizmoBase::ToolType::SMART_FILL) {
        m_smart_fill_angle_row->set_visible(true);
    } else if (m_selected_tool_type == PaintOnGizmoBase::ToolType::BUCKET_FILL) {
        m_bucket_fill_angle_row->set_visible(true);
    } else if (m_selected_tool_type == PaintOnGizmoBase::ToolType::HEIGHT_RANGE) {
        m_height_range_row->set_visible(true);
    }
}

} // namespace Slic3r::App::Plater
