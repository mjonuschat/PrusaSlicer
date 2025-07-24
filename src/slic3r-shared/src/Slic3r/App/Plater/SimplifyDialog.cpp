///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/SimplifyDialog.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/ProgressBar.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
#include "libslic3r/format.hpp"

#include <imgui_internal.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

SimplifyDialog::Callbacks& SimplifyDialog::callbacks()
{
    return m_callbacks;
}

SimplifyDialog::SimplifyDialog() : GizmoDialog(_u8L("Simplify"))
{
    content_item()->set_width(375);
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(2 * gap_size());

    Popup::callbacks().closed = [this]() {
        if (m_callbacks.close) {
            m_callbacks.close();
        }
    };

    m_mesh_name = Passthrough{std::make_unique<Text>("")};
    add_text_row(_u8L("Mesh name") + ":", m_mesh_name.release());

    m_triangles = Passthrough{std::make_unique<Text>("")};
    add_text_row(_u8L("Triangles") + ":", m_triangles.release());

    Dialog::add_separator();

    m_detail_level_btn = Passthrough{std::make_unique<RadioButton>(_u8L("Level of detail"))};
    m_detail_level     = Passthrough{std::make_unique<ComboBox>("Detail level")};
    m_detail_level->set_items(
        {_u8L("Extra high"), _u8L("High"), _u8L("Medium"), _u8L("Low"), _u8L("Extra low")}
    );
    m_detail_level->callbacks().selection_changed = [this](int index) {
        if (m_callbacks.detail_level_changed) {
            m_callbacks.detail_level_changed(index);
        }
    };
    add_radio_row(m_detail_level_btn.release(), m_detail_level.release());

    m_decimate_ratio_btn = Passthrough{std::make_unique<RadioButton>(
        _u8L("Decimate ratio"),
        _u8L(
            "A multipart object can be simplified using only a Level of detail. "
            "If you want to enter a Decimate ratio, do the simplification separately."
        )
    )};
    m_decimate_ratio     = Passthrough{std::make_unique<SliderWithInput>()};
    m_decimate_ratio->set_begin_value(0.);
    m_decimate_ratio->set_end_value(100.);
    m_decimate_ratio->set_input_width(40.f);
    m_decimate_ratio->set_validator_precision(2);
    m_decimate_ratio->callbacks().value_changed = [this](double value) {
        if (m_callbacks.decimate_ratio_changed) {
            m_callbacks.decimate_ratio_changed(value);
        }
    };
    add_radio_row(m_decimate_ratio_btn.release(), m_decimate_ratio.release(), "%");

    m_radio_group.set_buttons({m_detail_level_btn.get(), m_decimate_ratio_btn.get()});
    m_radio_group.callbacks().action = [this](AbstractButton* btn) {
        bool use_count = btn == m_decimate_ratio_btn.get();
        if (m_callbacks.use_count_changed) {
            m_callbacks.use_count_changed(use_count);
        }
    };

    m_info_line = content()->emplace_back<Text>("? triangles");
    m_info_line->set_self_align(YGAlignCenter);
    m_show_wireframe = content()->emplace_back<ToggleButton>(_u8L("Show wireframe"));
    m_show_wireframe->callbacks().checked_changed = [this](bool checked) {
        if (m_callbacks.show_wireframe_checked) {
            m_callbacks.show_wireframe_checked(checked);
        }
    };

    Item* row = content()->emplace_back<Item>();
    row->set_gap(2 * gap_size());
    m_apply_btn = row->emplace_back<LayoutButton>(_u8L("Apply"));
    m_apply_btn->set_background_color({43, 43, 43});
    m_apply_btn->callbacks().action = [this]() {
        if (m_callbacks.apply) {
            m_callbacks.apply();
        }
    };

    m_progress = row->emplace_back<ProgressBar>();
    m_progress->set_show_overlay(true);
    m_progress->set_flex_grow(1.f);
    m_progress->set_progress_fill(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
    m_progress->set_overlay_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
}

void SimplifyDialog::set_mesh_name(const std::string& name)
{
    m_mesh_name->set_text(name);
}

void SimplifyDialog::set_triangles(size_t triangles)
{
    m_triangles->set_text(std::to_string(triangles));
}

void SimplifyDialog::set_use_count(bool use_count)
{
    if (use_count) {
        m_decimate_ratio_btn->set_checked(true);
    } else {
        m_detail_level_btn->set_checked(true);
    }
    set_enabled_by_use_count(use_count);
}

void SimplifyDialog::set_detail_level(int index)
{
    m_detail_level->set_current_index(index);
}

void SimplifyDialog::set_decimate_ratio_step(double step)
{
    m_decimate_ratio->set_step(step);
}

void SimplifyDialog::set_decimate_ratio(double ratio)
{
    m_decimate_ratio->set_value(ratio);
}

void SimplifyDialog::set_info_line(const int wanted_triangles)
{
    m_info_line->set_text(Slic3r::format(_u8L("%1% triangles"), wanted_triangles));
}

void SimplifyDialog::set_show_wireframe(bool checked)
{
    m_show_wireframe->set_checked(checked);
}

void SimplifyDialog::set_enabled_by_use_count(bool use_count)
{
    m_detail_level->set_enabled(!use_count);
    m_decimate_ratio->set_enabled(use_count);
}

void SimplifyDialog::set_enable_apply_button(bool enable)
{
    m_apply_btn->set_enabled(enable);
    m_apply_btn->set_tooltip(enable ? "" : _u8L("Can't apply when proccess preview."));
}

void SimplifyDialog::set_enable_close_button(bool enable)
{
    close_button()->set_enabled(enable);
    close_button()->set_tooltip(
        enable ? "" : _u8L("Operation already cancelling. Please wait few seconds.")
    );
}

void SimplifyDialog::set_progress(int progress)
{
    m_progress->set_progress(progress, Slic3r::format(_u8L("Process %1%/100"), progress));
    m_progress->set_visible(progress < 100);
}

void SimplifyDialog::add_text_row(const std::string& title, std::unique_ptr<Yoga::Text> text_item)
{
    Item* row = content()->emplace_back<Item>();
    row->set_gap(2 * gap_size());

    Text* text = row->emplace_back<Text>(title);
    text->set_width_percent(25);
    text->set_self_align(YGAlignCenter);

    text_item->set_width_percent(65);
    text_item->set_text_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
    row->append(std::move(text_item));
}

void SimplifyDialog::add_radio_row(
    std::unique_ptr<Yoga::RadioButton> radio,
    Yoga::ItemPtr control,
    const std::string& unit
)
{
    Item* row = content()->emplace_back<Item>();
    row->set_gap(2 * gap_size());

    radio->set_width_percent(35);
    row->append(std::move(radio));

    Item* box = row->emplace_back<Item>();
    box->set_width_percent(65);
    control->set_flex_grow(1.f);
    box->append(std::move(control));

    if (!unit.empty()) {
        box->emplace_back<Text>(unit)->set_self_align(YGAlignCenter);
        box->set_gap(2 * gap_size());
    }
}
} // namespace Slic3r::App::Plater
