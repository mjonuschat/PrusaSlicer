#include "Slic3r/App/Plater/GizmoWindowWithLeftSidePanel.hpp"

#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

const constexpr float SIDE_PANEL_WIDTH = 80.f;

GizmoWindowWithLeftSidePanel::GizmoWindowWithLeftSidePanel(
    const std::string& title,
    Render::Icon icon,
    const std::string& shortcut
) :
    GizmoWindow(title, icon, shortcut)
{
    // Background rectangle for the side panel (with rounded corners on the left).
    m_side_panel = emplace<Rectangle>(0);
    m_side_panel->set_flex_shrink(0);
    m_side_panel->set_width(SIDE_PANEL_WIDTH);
    m_side_panel->set_orientation(Orientation::Vertical);
    m_side_panel->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));
    m_side_panel->set_flags(ImDrawFlags_RoundCornersLeft);
    m_side_panel->set_rounding(5.0f);

    m_side_panel_header_title = m_side_panel->emplace_back<Text>(std::string{});
    m_side_panel_header_title->set_self_align(YGAlign::YGAlignCenter);
}

Text* GizmoWindowWithLeftSidePanel::side_panel_header_title() const
{
    return m_side_panel_header_title;
}

Rectangle* GizmoWindowWithLeftSidePanel::side_panel() const
{
    return m_side_panel;
}

} // namespace Slic3r::App::Plater
