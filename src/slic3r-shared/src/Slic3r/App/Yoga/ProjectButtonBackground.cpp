#include "Slic3r/App/Yoga/ProjectButtonBackground.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "imgui/imgui_internal.h"

namespace Slic3r::App::Yoga {

ProjectButtonBackground::ProjectButtonBackground() : Rectangle() {}

void ProjectButtonBackground::render(Vec2f pos, Vec2f size)
{
    if (m_mode == Mode::FilledRect) {
        Rectangle::render(pos, size);
        return;
    }

    // m_mode == Mode::Border

    render_item_begin(pos, size);

    const bool is_enabled = enabled();

    ImRect rect(to_im(pos), to_im(pos + size));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ASSERT(draw_list);

    ImColor fill_color;
    if (enabled()) {
        fill_color = fill();
    }
    else {
        fill_color = IM_COL32_DISABLE;
    }

    ImVec2 tl = rect.Min;
    ImVec2 tr = ImVec2(rect.Max.x, tl.y);
    ImVec2 bl = ImVec2(tl.x, rect.Max.y);
    ImVec2 br = rect.Max;

    // Outer left vertical bar
    draw_list->AddRectFilled(tl, ImVec2(tl.x + m_thickness.x(), bl.y), fill_color);
    // Outer right vertical bar
    draw_list->AddRectFilled(ImVec2(tr.x - m_thickness.x(), tr.y), br, fill_color);
    // Top bar
    draw_list->AddRectFilled(ImVec2(tl.x + m_thickness.x(), tl.y), ImVec2(tr.x - m_thickness.x(), tl.y + m_thickness.y()), fill_color);

    // Rounded corners (top inner left and right)
    ImVec2 left_corner_center = ImVec2(tl.x + m_thickness.x() + m_inner_rounding, tl.y + m_thickness.y() + m_inner_rounding);
    ImVec2 right_corner_center = ImVec2(tr.x - m_thickness.x() - m_inner_rounding, tl.y + m_thickness.y() + m_inner_rounding);

    draw_list->PathLineTo(rect.Min);
    draw_list->PathArcTo(left_corner_center, m_inner_rounding, IM_PI, IM_PI * 1.5f, 8);
    draw_list->PathLineTo({ rect.Max.x, rect.Min.y });
    draw_list->PathFillConvex(fill_color);

    draw_list->PathLineTo(tr);
    draw_list->PathArcTo(right_corner_center, m_inner_rounding, IM_PI * 1.5f, IM_PI * 2.0f, 8);
    draw_list->PathLineTo(tr);
    draw_list->PathFillConvex(fill_color);

    render_item_end(pos, size);
}

ProjectButtonBackground::Mode ProjectButtonBackground::mode() const { return m_mode; }

void ProjectButtonBackground::set_mode(Mode mode) { m_mode = mode; }

float ProjectButtonBackground::inner_rounding() const { return m_inner_rounding; }

void ProjectButtonBackground::set_inner_rounding(float rounding) { m_inner_rounding = rounding; }

Vec2f ProjectButtonBackground::thickness() const { return m_thickness; }

void ProjectButtonBackground::set_thichness(Vec2f thickness) { m_thickness = thickness; }

} // namespace Slic3r::App::Yoga