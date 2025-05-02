#include "Slic3r/App/Yoga/Rectangle.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App::Yoga {

Rectangle::Rectangle(Item* parent) : Item(parent) {}

void Rectangle::render(Vec2f pos, Vec2f size)
{
    if (!m_parent) {
        style_node();
        resize(size);
    }

    ImRect rect(to_im(pos), to_im(pos + size));

    bool hovered = ImGui::IsMouseHoveringRect(rect.Min, rect.Max);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ASSERT(draw_list);

    ImColor fill_color = hovered && m_hover_effect ? Imgui::adjust_brightness(m_fill, 1.2) : m_fill;

    draw_list->AddRectFilled(rect.Min, rect.Max, fill_color, m_rounding);
    if (m_border_width > 0) {
        draw_list->AddRect(rect.Min, rect.Max, m_border_color, m_rounding, 0, m_border_width);
    }

    render_internal(pos, size);
}

const ImColor& Rectangle::fill() const { return m_fill; }

const ImColor& Rectangle::border_color() const { return m_border_color; }

float Rectangle::border_width() const { return m_border_width; }

float Rectangle::rounding() const { return m_rounding; }

bool Rectangle::hover_effect() const { return m_hover_effect; }

void Rectangle::set_fill(const ImColor& fill) { m_fill = fill; }

void Rectangle::set_border_color(const ImColor& border_color) { m_border_color = border_color; }

void Rectangle::set_border_width(float border_width) { m_border_width = border_width; }

void Rectangle::set_rounding(float rounding) { m_rounding = rounding; }

void Rectangle::set_hover_effect(bool hover_effect) { m_hover_effect = hover_effect; }

} // namespace Slic3r::App::Yoga
