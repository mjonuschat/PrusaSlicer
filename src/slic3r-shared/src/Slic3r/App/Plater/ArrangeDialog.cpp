#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Slider.hpp"
#include "Slic3r/App/Yoga/SliderWithInput.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/App/Plater/ArrangeDialog.hpp"

namespace Slic3r::App::Plater {

using Biz::Algorithms::Scaling::scaled;
using Biz::Algorithms::Scaling::unscaled;
using Biz::Arrange::GeometryHandling;
using Biz::Arrange::PivotPoint;
using Biz::Arrange::Settings;
using Domain::BedRefs;
using Domain::coord_t;
using Domain::SelectionId;
using Domain::Vec2f;
using Yoga::AbstractButton;
using Yoga::ButtonGroup;
using Yoga::Item;
using Yoga::ItemPtr;
using Yoga::LayoutButton;
using Yoga::Orientation;
using Yoga::Slider;
using Yoga::SliderWithInput;
using Yoga::Text;
using Yoga::ToggleButton;

namespace {
std::optional<PivotPoint> get_pivot_point(const std::size_t row, const std::size_t column)
{
    if (row == 0 && column == 0) {
        return PivotPoint::TopLeft;
    }
    if (row == 0 && column == 2) {
        return PivotPoint::TopRight;
    }
    if (row == 1 && column == 1) {
        return PivotPoint::Center;
    }
    if (row == 2 && column == 0) {
        return PivotPoint::BottomLeft;
    }
    if (row == 2 && column == 2) {
        return PivotPoint::BottomRight;
    }
    return std::nullopt;
}

Render::Icon get_icon(const PivotPoint pivot)
{
    switch (pivot) {
    case PivotPoint::TopLeft:
        return Render::Icon::ArrangeTopLeft;
    case PivotPoint::TopRight:
        return Render::Icon::ArrangeTopRight;
    case PivotPoint::BottomLeft:
        return Render::Icon::ArrangeBottomLeft;
    case PivotPoint::BottomRight:
        return Render::Icon::ArrangeBottomRight;
    case PivotPoint::Center:
        return Render::Icon::ArrangeCenter;
    }
    PANIC("Invalid pivot");
}

} // namespace

class PivotPicker : public Item
{
public:
    PivotPicker()
    {
        const float button_size{16.0};
        set_orientation(Orientation::Vertical);
        for (std::size_t row_index{}; row_index < 3; ++row_index) {
            Item* row = emplace_back<Item>();
            for (std::size_t column_index{}; column_index < 3; ++column_index) {
                const std::optional<PivotPoint> pivot_point{get_pivot_point(row_index, column_index)};
                if (pivot_point) {
                    LayoutButton* button{row->emplace_back<LayoutButton>("", get_icon(*pivot_point))};
                    button->set_rounding(0.0);
                    button->set_content_padding(0.0);
                    button->set_background_color(IM_COL32_BLACK_TRANS);
                    button->set_checkable(true);
                    button->set_width(button_size);
                    button->set_height(button_size);
                    if (pivot_point == PivotPoint::Center) {
                        button->set_checked(true);
                    }
                    m_group.insert_button(button);
                    m_pivot_buttons.insert({button, *pivot_point});
                } else {
                    Item* filler{row->emplace_back<Item>()};
                    filler->set_width(button_size);
                    filler->set_height(button_size);
                }
            }
        }
    }

    PivotPoint get_selected_pivot() const
    {
        return m_pivot_buttons.at(m_group.checked_button());
    }

private:
    ButtonGroup m_group;
    std::map<AbstractButton*, PivotPoint> m_pivot_buttons;
};

ArrangeDialog::ArrangeDialog(OnArrange on_arrange, const Settings& settings) :
    Yoga::GizmoDialog{_u8L("Arrange")},
    m_on_arrange{on_arrange}
{
    content_item()->set_width(325.f);
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(gap_size());

    auto offset_slider{std::make_unique<SliderWithInput>()};
    offset_slider->set_begin_value(0.0);
    offset_slider->set_end_value(100.0);
    offset_slider->set_step(1.0);

    m_offset_slider = offset_slider.get();
    GizmoDialog::add_new_row(_u8L("Spacing"), std::move(offset_slider));
    m_offset_slider->set_value(unscaled(static_cast<coord_t>(settings.scaled_offset)) * 2.0);

    auto bed_offset_slider{std::make_unique<SliderWithInput>()};
    bed_offset_slider->set_begin_value(0.0);
    bed_offset_slider->set_end_value(100.0);
    bed_offset_slider->set_step(1.0);

    m_bed_offset_slider = bed_offset_slider.get();
    GizmoDialog::add_new_row(_u8L("Bed spacing"), std::move(bed_offset_slider));
    m_bed_offset_slider->set_value(settings.unscaled_bed_offset);

    auto geometry_handling_control{std::make_unique<Item>()};
    geometry_handling_control->set_align_content(YGAlign::YGAlignCenter);
    geometry_handling_control->set_gap(15.0);
    geometry_handling_control->emplace_back<Text>("fast");
    m_mode_slider = geometry_handling_control->emplace_back<Slider>(0.0, 2.0, 1.0);
    m_mode_slider->set_flex_grow(1.0);
    geometry_handling_control->emplace_back<Text>("accurate");
    add_new_row("Mode", std::move(geometry_handling_control));

    Dialog::add_separator();

    auto pivot_picker{std::make_unique<PivotPicker>()};
    m_pivot_picker = pivot_picker.get();
    m_bed_segments_row = add_new_row("Alignment", std::move(pivot_picker), YGAlign::YGAlignFlexStart);
    m_bed_segments_separator = Dialog::add_separator();
    set_bed_segments(settings.bed_segments);

    m_enable_rotations_toggle = content()->emplace_back<ToggleButton>("Enable rotations");
    m_enable_rotations_toggle->set_checked(settings.allow_rotations);

    Dialog::add_separator();

    LayoutButton* button{content()->emplace_back<LayoutButton>(_u8L("Arrange"))};
    button->callbacks().action = [this]() {
        m_on_arrange(get_settings());
    };

    button->set_flex_grow(1);

    constexpr ImColor color_primary{223, 93, 45};
    button->set_background_color(color_primary);
    button->set_label_font_type(Render::ImguiFontType::Bold);
}

void ArrangeDialog::set_bed_segments(const std::optional<Domain::Bed::Segments>& bed_segments)
{
    m_bed_segments = bed_segments;

    if (bed_segments) {
        m_bed_segments_row->set_visible(true);
        m_bed_segments_separator->set_visible(true);
    } else if (!bed_segments) {
        m_bed_segments_row->set_visible(false);
        m_bed_segments_separator->set_visible(false);
    }
}

Settings ArrangeDialog::get_settings() const
{
    Settings result;
    result.scaled_offset       = scaled(m_offset_slider->value() / 2.0);
    result.unscaled_bed_offset = m_bed_offset_slider->value();

    const double mode_value{m_mode_slider->value()};
    if (mode_value < 0.5) {
        result.fixed_geometry   = GeometryHandling::Convex;
        result.movable_geometry = GeometryHandling::Convex;
    } else if (mode_value < 1.5) {
        result.fixed_geometry   = GeometryHandling::Arbitrary;
        result.movable_geometry = GeometryHandling::Convex;
    } else {
        result.fixed_geometry   = GeometryHandling::Arbitrary;
        result.movable_geometry = GeometryHandling::Arbitrary;
    }

    if (m_bed_segments) {
        ASSERT(m_pivot_picker != nullptr);
        result.bed_pivot_point = m_pivot_picker->get_selected_pivot();
        result.bed_segments    = m_bed_segments;
    }

    result.allow_rotations = m_enable_rotations_toggle->checked();

    return result;
}

} // namespace Slic3r::App::Plater
