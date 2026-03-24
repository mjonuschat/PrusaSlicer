///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ToolLabel.hpp"

#include "Slic3r/App/Config/ConfigItemUtils.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include <imgui_internal.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ToolLabel::ToolLabel(size_t index, const bool& data) :
    Biz::DataObserver<bool>(index, data),
    LayoutButton(std::string{}, Render::Icon::Funnel)
{
    set_margin(4);
    set_background_border_width(1);
    set_background_color(IM_COL32_BLACK_TRANS, false);
    set_rounding(2);
    set_tooltip_position(Position::Top);

    on_index_update();
}

void ToolLabel::on_index_update()
{
    set_label(std::to_string(m_index + 1));
    update_markings();
}

void ToolLabel::on_data_update()
{
    update_markings();
}

void ToolLabel::update_markings()
{
    ImColor im_color;
    std::string tooltip_text;
    const std::string index_str = std::to_string(m_index + 1);
    if (*m_state) {
        im_color     = ConfigItemUtils::colors.at(m_index);
        tooltip_text = Biz::_u8L("Tool " + index_str + " is used on this bed");
    } else {
        im_color     = m_theme->color_imgui(Platform::Color::Text, Platform::ColorGroup::Disabled);
        tooltip_text = Biz::_u8L("Tool " + index_str + " is not used on this bed");
    }
    set_tooltip(tooltip_text);
    set_icon_tint(im_color);
    set_background_color_border(im_color, false);
}

} // namespace Slic3r::App
