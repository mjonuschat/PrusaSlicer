///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Plater/WarningPanel.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

#include <numeric>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

WarningPanel::WarningPanel()
{
    const ImColor warning_color = m_theme->color_imgui(Platform::Color::Warning);
    ImColor warning_color_fill  = warning_color;
    warning_color_fill.Value.w  = 0.15; // 15%
    set_fill(warning_color_fill);
    set_rounding(0);

    set_orientation(Orientation::Vertical);

    emplace_back<Separator>();

    Item* content_row = emplace_back<Item>();
    content_row->set_gap(10);
    content_row->set_padding(10);

    m_warning_icon = content_row->emplace_back<Icon>(Render::Icon::ExclamationRed);
    m_warning_icon->set_margin(Margins{5, 2, 0, 0});
    m_warning_icon->set_width(16);
    m_warning_icon->set_height(16);
    m_warning_icon->set_aspect_ratio(1);
    m_warning_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
    m_warning_icon->set_self_align(YGAlign::YGAlignFlexStart);

    Item* text_column = content_row->emplace_back<Item>();
    text_column->set_gap(5);
    text_column->set_flex_grow(1);
    text_column->set_orientation(Orientation::Vertical);

    m_title = text_column->emplace_back<Text>(std::string{});
    m_title->set_font_type(Render::ImguiFontType::Bold);
    m_title->set_text_color(warning_color);

    m_text = text_column->emplace_back<Text>(std::string{});
    m_text->set_wrap_mode(Text::WrapMode::Wrap);
    m_text->set_text_color(warning_color);

    emplace_back<Separator>();
}

void WarningPanel::set_warning(const std::string& title, const std::string& text)
{
    m_title->set_text(title);
    m_text->set_text(text);
}

void WarningPanel::set_warning(const std::string& title, const std::vector<std::string>& errors)
{
    m_title->set_text(title);

    m_text->set_text(
        std::accumulate(
            std::next(errors.begin()),
            errors.end(),
            errors.empty() ? std::string{} : errors.front(),
            [](std::string sum, const std::string& error) { return sum + '\n' + error; }
        )
    );
}

} // namespace Slic3r::App::Plater
