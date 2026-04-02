///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ToolLabel.hpp"

#include "Slic3r/Biz/Algorithms/Color.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <imgui_internal.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ToolLabel::ToolLabel(size_t index, const bool& data, Biz::ProjectInteractor& project_interactor) :
    Biz::DataObserver<bool>(index, data),
    LayoutButton(std::string{}, Render::Icon::Funnel),
    m_project_interactor(project_interactor),
    m_colors_changed_listener_scope(project_interactor.project_settings_interactor(), *this)
{
    set_margin(4);
    set_background_border_width(1);
    set_background_color(IM_COL32_BLACK_TRANS, false);
    set_rounding(2);
    set_tooltip_position(Position::Top);

    on_index_update();

    update_markings();
}

void ToolLabel::on_colors_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    const std::vector<Domain::ColorRGB>& colors
)
{
    update_markings();
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
        const std::vector<Domain::ColorRGB> colors =
            m_project_interactor.project_settings_interactor().get_colors(
                m_project_interactor.selected_config_container_id()
            );
        if (colors.size() > m_index) {
            const Domain::ColorRGB& color = colors[m_index];
            im_color                      = {color.r(), color.g(), color.b()};
        } else {
            Domain::ColorRGB color;
            ASSERT(
                Biz::Algorithms::Color::decode_color(
                    Biz::ProjectSettingsInteractor::palette_color(m_index),
                    color
                )
            );
            im_color = {color.r(), color.g(), color.b()};
        }
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
