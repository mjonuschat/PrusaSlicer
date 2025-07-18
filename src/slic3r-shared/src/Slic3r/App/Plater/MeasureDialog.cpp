///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/MeasureDialog.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/Math.hpp"

#include "libslic3r/format.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Plater::Measure;

namespace Slic3r::App::Plater {

MeasureDialog::MeasureDialog() : GizmoDialog(_u8L("Measure"))
{
    const Vec2f shortcut_button_size{30.f, 30.f};

    content_item()->set_width(325);
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(gap_size());

    // Create help panel

    m_helper_panel = content()->emplace_back<Item>();
    m_helper_panel->set_orientation(Orientation::Vertical);
    m_helper_panel->set_align_items(YGAlign::YGAlignFlexStart);
    m_helper_panel->set_gap(gap_size());

    m_extra_help.init(m_helper_panel);
    m_extra_help.add_item({ {Render::Icon::MouseLeft} }, _u8L("Select Feature"), false);
    m_extra_help.add_item({ {Render::Icon::KeyEsc, shortcut_button_size} }, _u8L("Unselect last"), false);
    m_extra_help.add_item({ {Render::Icon::KeyDel, shortcut_button_size} }, _u8L("Restart"), false);

    // Create main panel

    m_main_panel = content()->emplace_back<Item>();
    m_main_panel->set_orientation(Orientation::Vertical);
    m_main_panel->set_align_content(YGAlign::YGAlignFlexStart);
    m_main_panel->set_gap(gap_size());

    // Create measure item
    m_measures_item = m_main_panel->emplace_back<Item>();
    m_measures_item->set_orientation(Orientation::Vertical);

    add_separator(m_main_panel);

    std::unique_ptr<SpotDescription> spot1 = std::make_unique<SpotDescription>();
    m_spot1                                = spot1.get();
    add_spot_row(ImColor(64, 191, 191), _u8L("Spot 1"), std::move(spot1));

    std::unique_ptr<SpotDescription> spot2 = std::make_unique<SpotDescription>();
    m_spot2                                = spot2.get();
    add_spot_row(ImColor(191, 64, 191), _u8L("Spot 2"), std::move(spot2));

    add_separator(m_main_panel);

    Item* help_row = m_main_panel->emplace_back<Item>();
    help_row->set_min_size({0, 50});
    help_row->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row->set_align_content(YGAlign::YGAlignCenter);
    help_row->set_padding(5);
    help_row->set_gap(15);

    m_help.init(help_row);
    m_help.add_item({ {Render::Icon::MouseLeft} }, _u8L("Select"));
    m_help.add_item({ {Render::Icon::KeyEsc, shortcut_button_size} }, _u8L("Unselect"));
    m_help.add_item({ {Render::Icon::KeyDel, shortcut_button_size}  }, _u8L("Restart"));

    m_main_panel->set_visible(false);

    // The code below this comment is just for testing and will be removed after the MeasureGizmo implementation.
    add_separator(m_main_panel);

    auto test_btn = content()->emplace_back<LayoutButton>("Select feature");
    test_btn->set_checkable(true);
    test_btn->set_background_color(IM_COL32_BLACK_TRANS);
    test_btn->callbacks().checked_changed = [this, test_btn](bool checked) {
        show_measure(checked);
        test_btn->set_label(checked ? "Restart" : "Select feature");
    };
}

void MeasureDialog::SpotDescription::reset()
{
    m_name->set_text("");
    m_value->set_text("");
}

void MeasureDialog::SpotDescription::set_from(const FeatureItem& feature)
{
    switch (feature.feature.type()) {
    case SurfaceFeatureType::Undefined: {
        reset();
        break;
    }
    case SurfaceFeatureType::Point: {
        if (feature.parent.has_value()) {
            std::string parent_type;
            switch (feature.parent->type()) {
            case SurfaceFeatureType::Edge: {
                parent_type = _u8L("edge");
                break;
            }
            case SurfaceFeatureType::Circle: {
                parent_type = _u8L("circle");
                break;
            }
            case SurfaceFeatureType::Plane: {
                parent_type = _u8L("plane");
                break;
            }
            default:
                PANIC("invalid parent");
            }
            m_name->set_text(Slic3r::format("%1% %2%", _u8L("Point on"), parent_type));
        } else
            m_name->set_text(_u8L("Vertex"));
        m_value->set_text("");
        break;
    }
    case SurfaceFeatureType::Edge: {
        auto [from, to] = feature.feature.edge();
        m_name->set_text(_u8L("Edge"));
        m_value->set_text(Slic3r::format("%1%: %2$.3f mm", _u8L("Length"), (to - from).norm()));
        break;
    }
    case SurfaceFeatureType::Circle: {
        auto [center, radius, normal] = feature.feature.circle();
        m_name->set_text(_u8L("Circle"));
        m_value->set_text(Slic3r::format("%1%: %2$.3f mm", _u8L("Diameter"), 2.0f * float(radius)));
        break;
    }
    case SurfaceFeatureType::Plane: {
        m_name->set_text(_u8L("Plane"));
        m_value->set_text("");
        break;
    }
    }
}

MeasureDialog::SpotDescription& MeasureDialog::spot1()
{
    return *m_spot1;
}

MeasureDialog::SpotDescription& MeasureDialog::spot2()
{
    return *m_spot2;
}

void MeasureDialog::set_measure(const MeasurementResult& result)
{
    for (auto i : m_measure_rows) {
        m_measures_item->remove(i);
    }
    m_measure_rows.clear();

    bool show_strict = result.distance_strict.has_value()
        && (!result.distance_infinite.has_value()
            || std::abs(result.distance_strict->dist - result.distance_infinite->dist)
                > Domain::EPSILON);

    if (result.angle.has_value())
        add_measure_row(_u8L("Angle"), Slic3r::format("%1$.3f", rad2deg(result.angle->angle)), _u8L("°"));
    if (result.distance_infinite.has_value())
        add_measure_row(
            show_strict ? _u8L("Perpendicular distance") : _u8L("Distance"),
            Slic3r::format("%1$.3f", result.distance_infinite->dist),
            _u8L("mm")
        );
    if (show_strict)
        add_measure_row(
            _u8L("Direct distance"),
            Slic3r::format("%1$.3f", result.distance_strict->dist),
            _u8L("mm")
        );
    if (result.distance_xyz.has_value())
        add_measure_row(
            _u8L("Distance XYZ"),
            Slic3r::format(
                "x:%1$.3f y:%2$.3f z:%3$.3f",
                result.distance_xyz->x(),
                result.distance_xyz->y(),
                result.distance_xyz->z()
            ),
            _u8L("mm")
        );
}

void MeasureDialog::show_measure(bool show)
{
    m_helper_panel->set_visible(!show);
    m_main_panel->set_visible(show);
}

void MeasureDialog::add_measure_row(
    const std::string& name,
    const std::string& value,
    const std::string& units
)
{
    Item* row = m_measure_rows.emplace_back(m_measures_item->emplace_back<Item>());
    row->set_gap(gap_size());
    row->set_padding({content()->padding().left, 0});
    MeasureRowItem& row_item = m_measure_row_items.emplace_back();
    row_item.name            = row->emplace_back<Text>(name);
    row_item.name->set_self_align(YGAlignCenter);
    row_item.name->set_width_percent(35);

    Item* value_item = row->emplace_back<Item>();
    value_item->set_width_percent(65);
    value_item->set_gap(gap_size());
    row_item.value = value_item->emplace_back<Text>(Slic3r::format("%1% %2%", value, units));
    row_item.value->set_self_align(YGAlignCenter);
    row_item.value->set_font_type(Render::ImguiFontType::Bold);
    row_item.value->set_flex_grow(1);

    row_item.copy_btn = value_item->emplace_back<LayoutButton>("", Render::Icon::CopyForGizmo);
    row_item.copy_btn->set_background_color(IM_COL32_BLACK_TRANS);
    row_item.clipboard_text               = Slic3r::format("%1%: %2% %3%", name, value, units);
    size_t id                             = m_measure_row_items.size() - 1;
    row_item.copy_btn->callbacks().action = [this, id]() {
        ImGui::SetClipboardText(m_measure_row_items[id].clipboard_text.c_str());
    };
}

void MeasureDialog::add_spot_row(const ImColor& marker_color, const std::string& title, Yoga::ItemPtr controls)
{
    Item* row = m_main_panel->emplace_back<Item>();
    row->set_gap(gap_size());
    row->set_padding({content()->padding().left, 0});

    Item* label_with_marker = row->emplace_back<Item>();
    label_with_marker->set_gap(gap_size());
    label_with_marker->set_width_percent(35);

    Circle* marker = label_with_marker->emplace_back<Circle>();
    marker->set_height(16);
    marker->set_fill(marker_color);
    Text* text = label_with_marker->emplace_back<Text>(title);

    controls->set_width_percent(65);
    row->append(std::move(controls));
}

MeasureDialog::SpotDescription::SpotDescription() : Item()
{
    set_orientation(Yoga::Orientation::Vertical);
    set_gap(0);
    m_name = emplace_back<Text>(_u8L(""));
    m_name->set_font_type(Render::ImguiFontType::Bold);
    m_value = emplace_back<Text>("");
    m_value->set_flex_grow(1);
}

} // namespace Slic3r::App::Plater
