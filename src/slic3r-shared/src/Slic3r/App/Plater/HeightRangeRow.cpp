#include "Slic3r/App/Plater/HeightRangeRow.hpp"

#include "Slic3r/App/Render/ImguiTypes.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

const constexpr ImColor HEIGHT_RANGE_ROW_SELECTED_COLOR   = ImColor(54, 73, 118, 255);
const constexpr ImColor HEIGHT_RANGE_ROW_UNSELECTED_COLOR = ImColor(42, 42, 42, 255);
const constexpr ImColor HEIGHT_RANGE_ROW_HOVERED_COLOR    = ImColor(60, 60, 60, 255);
const constexpr float HEIGHT_RANGE_ROW_ROUNDING           = 3.f;
const constexpr float HEIGHT_RANGE_ROW_HEIGHT             = 26.f;

HeightRangeRow::HeightRangeRow(const HeightRangeEntry& height_range) : m_height_range(height_range)
{
    this->set_object_name("HeightRangeRow");
    this->set_allow_overlap(true);
    this->set_flex_shrink(0);
    this->set_orientation(Orientation::Horizontal);
    this->set_height(HEIGHT_RANGE_ROW_HEIGHT);
    this->set_gap(4.f);

    m_background = emplace_back<Rectangle>();
    m_background->set_rounding(HEIGHT_RANGE_ROW_ROUNDING);
    m_background->set_flex_grow(1);
    m_background->set_orientation(Orientation::Horizontal);
    m_background->set_padding({8.f, 4.f, 8.f, 4.f});
    m_background->set_gap(8.f);
    m_background->set_align_items(YGAlignCenter);

    m_range_label = m_background->emplace_back<Text>("");
    m_range_label->set_font_type(Render::ImguiFontType::Bold);
    m_range_label->set_flex_grow(1);

    m_height_label = m_background->emplace_back<Text>("");

    m_undo_button = emplace_back<LayoutButton>("", Render::Icon::UndoGizmo);
    m_undo_button->set_min_size(Vec2f(22, 22));
    m_undo_button->set_self_align(YGAlignCenter);
    m_undo_button->set_flex_shrink(0);
    m_undo_button->set_background_color(IM_COL32_BLACK_TRANS);
    m_undo_button->callbacks().action = [this]()
    {
        if (m_has_overrides) {
            m_callbacks.undo_clicked();
        }
    };

    m_delete_button = emplace_back<LayoutButton>("", Render::Icon::Minus);
    m_delete_button->set_min_size(Vec2f(22, 22));
    m_delete_button->set_self_align(YGAlignCenter);
    m_delete_button->set_flex_shrink(0);
    m_delete_button->set_background_color(IM_COL32_BLACK_TRANS);
    m_delete_button->callbacks().action = [this]() { m_callbacks.delete_clicked(); };

    AbstractButton::callbacks().action = [this]()
    {
        if (m_delete_button->hovered() || m_undo_button->hovered()) {
            return;
        }

        m_callbacks.selected();
    };

    this->update_background();
    this->update_labels();
    this->update_undo_button();
}

HeightRangeRow::Callbacks& HeightRangeRow::callbacks()
{
    return m_callbacks;
}

const HeightRangeEntry& HeightRangeRow::height_range() const
{
    return m_height_range;
}

void HeightRangeRow::set_height_range(const HeightRangeEntry& height_range)
{
    m_height_range = height_range;
    this->update_background();
    this->update_labels();
}

void HeightRangeRow::set_has_overrides(const bool has_overrides)
{
    m_has_overrides = has_overrides;
    this->update_undo_button();
}

void HeightRangeRow::set_selected(const bool selected)
{
    m_selected = selected;
    this->update_background();
}

void HeightRangeRow::set_highlighted(const bool highlighted)
{
    m_highlighted = highlighted;
    this->update_background();
}

void HeightRangeRow::hovered_updated_internal()
{
    m_callbacks.hovered(hovered());
    this->update_background();
}

void HeightRangeRow::update_background()
{
    if (m_selected) {
        m_background->set_fill(HEIGHT_RANGE_ROW_SELECTED_COLOR);
    } else if (this->hovered() || m_highlighted) {
        m_background->set_fill(HEIGHT_RANGE_ROW_HOVERED_COLOR);
    } else {
        m_background->set_fill(HEIGHT_RANGE_ROW_UNSELECTED_COLOR);
    }
}

void HeightRangeRow::update_labels()
{
    const auto format_trimmed = [](double value, int precision) -> std::string
    {
        std::string value_str = fmt::format("{:.{}f}", value, precision);
        if (value_str.find('.') != std::string::npos) {
            value_str.erase(value_str.find_last_not_of('0') + 1, std::string::npos);
            if (value_str.back() == '.') {
                value_str.pop_back();
            }
        }

        return value_str;
    };

    m_range_label->set_text(
        format_trimmed(m_height_range.min_z, 3)
        + " - "
        + format_trimmed(m_height_range.max_z, 3)
        + " mm"
    );
    m_height_label->set_text(format_trimmed(m_height_range.layer_height, 2) + " mm");
}

void HeightRangeRow::update_undo_button()
{
    if (m_has_overrides) {
        m_undo_button->set_icon(Render::Icon::UndoGizmo);
        m_undo_button->set_background_color(IM_COL32_BLACK_TRANS);
    } else {
        m_undo_button->set_icon(Render::Icon::None);
        m_undo_button->set_background_color(IM_COL32_BLACK_TRANS, false);
    }
}

} // namespace Slic3r::App::Plater
