///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/MeasureDialog.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
#include "libslic3r/format.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

constexpr float gap_size = 10;

MeasureDialog::MeasureDialog() : GizmoDialog(_u8L("Measure"))
{
    const Vec2f shortcut_button_size{30.f, 30.f};

    content_item()->set_width(325);
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(gap_size);

    // Create help panel

    m_helper_panel = content()->emplace_back<Item>();
    m_helper_panel->set_orientation(Orientation::Vertical);
    m_helper_panel->set_align_items(YGAlign::YGAlignFlexStart);
    m_helper_panel->set_gap(gap_size);

    add_help({ {Render::Icon::MouseLeft} }, _u8L("Select Feature"), m_helper_panel, false);
    add_help({ {Render::Icon::KeyEsc, shortcut_button_size} }, _u8L("Unselect last"), m_helper_panel, false);
    add_help({ {Render::Icon::KeyDel, shortcut_button_size} }, _u8L("Restart"), m_helper_panel, false);

    // Create main panel

    m_main_panel = content()->emplace_back<Item>();
    m_main_panel->set_orientation(Orientation::Vertical);
    m_main_panel->set_align_content(YGAlign::YGAlignFlexStart);
    m_main_panel->set_gap(gap_size);

    add_measure_row();

    add_separator(m_main_panel);

    std::unique_ptr<SpotDescription> spot1 = std::make_unique<SpotDescription>();
    m_spot1 = spot1.get();
    add_spot_row(ImColor(191,64,191), _u8L("Spot 1"), std::move(spot1));

    std::unique_ptr<SpotDescription> spot2 = std::make_unique<SpotDescription>();
    m_spot2 = spot2.get();
    add_spot_row(ImColor(64,191,191), _u8L("Spot 2"), std::move(spot2));

    add_separator(m_main_panel);

    Item* help_row = m_main_panel->emplace_back<Item>();
    help_row->set_min_size({0, 50});
    help_row->set_justify_content(YGJustify::YGJustifySpaceEvenly);
    help_row->set_align_content(YGAlign::YGAlignCenter);
    help_row->set_padding(5);
    help_row->set_gap(15);

    add_help({ {Render::Icon::MouseLeft} }, _u8L("Select"), help_row);
    add_help({ {Render::Icon::KeyEsc, shortcut_button_size} }, _u8L("Unselect"), help_row);
    add_help({ {Render::Icon::KeyDel, shortcut_button_size}  }, _u8L("Restart"), help_row);

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

MeasureDialog::SpotDescription& MeasureDialog::spot1()
{
    return *m_spot1;
}

MeasureDialog::SpotDescription& MeasureDialog::spot2()
{
    return *m_spot2;
}

void MeasureDialog::set_measure(MeasureType type, float value)
{
    if (type == MeasureType::Angle) {
        m_measure_name->set_text(_u8L("Angle"));
        m_measure_value->set_text(Slic3r::format("%1$.3f °", value));
    }
    else if (type == MeasureType::Distance) {
        m_measure_name->set_text(_u8L("Distance"));
        m_measure_value->set_text(Slic3r::format("%1$.3f mm", value));
    }
}

void MeasureDialog::show_measure(bool show)
{
    m_helper_panel->set_visible(!show);
    m_main_panel->set_visible(show);
}

std::function<void()>& MeasureDialog::on_copy()
{
    return m_copy_btn->callbacks().action;
}

void MeasureDialog::add_measure_row()
{
    Item* row = m_main_panel->emplace_back<Item>();
    row->set_gap(gap_size);
    row->set_padding({ 10, 0 });
    m_measure_name = row->emplace_back<Text>("");
    m_measure_name->set_self_align(YGAlignCenter);
    m_measure_name->set_width_percent(35);

    Item* value_item = row->emplace_back<Item>();
    value_item->set_width_percent(65);
    value_item->set_gap(gap_size);
    m_measure_value = value_item->emplace_back<Text>("");
    m_measure_value->set_self_align(YGAlignCenter);
    m_measure_value->set_font_type(Render::ImguiFontType::Bold);
    m_measure_value->set_text_color(ImColor({ 192,154,247 }));
    m_measure_value->set_flex_grow(1);

    m_copy_btn = value_item->emplace_back<LayoutButton>("", Render::Icon::CopyForGizmo);
    m_copy_btn->set_background_color(IM_COL32_BLACK_TRANS);
}

void MeasureDialog::add_spot_row(const ImColor& marker_color, const std::string& title, std::unique_ptr<Item> controls)
{
    Item* row = m_main_panel->emplace_back<Item>();
    row->set_gap(gap_size);
    row->set_padding({10, 0});

    Item* label_with_marker = row->emplace_back<Item>();
    label_with_marker->set_gap(gap_size);
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

void MeasureDialog::SpotDescription::set_as_plane()
{
    m_name->set_text(_u8L("Plane"));
    m_value->set_text("");
}

void MeasureDialog::SpotDescription::set_as_edge(float lenth)
{
    m_name->set_text(_u8L("Edge"));
    m_value->set_text(Slic3r::format("%1%: %2$.3f mm", _u8L("Length"), lenth));
}

} // namespace Slic3r::App::Plater
